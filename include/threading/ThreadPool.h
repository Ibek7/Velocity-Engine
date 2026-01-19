#pragma once

#include <vector>
#include <queue>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <array>
#include <optional>

namespace JJM {
namespace Threading {

// =============================================================================
// Advanced Job System with Task Graphs, Priorities, and Dependencies
// =============================================================================

// Task priority levels
enum class TaskPriority {
    Critical = 0,   // Highest priority - game loop critical
    High = 1,       // Important tasks - rendering, physics
    Normal = 2,     // Standard priority - general game logic
    Low = 3,        // Background tasks - asset loading
    Idle = 4,       // Lowest priority - analytics, optional tasks
    Count
};

// Task affinity for thread binding
enum class TaskAffinity {
    Any,            // Can run on any thread
    MainThread,     // Must run on main thread
    RenderThread,   // Must run on render thread
    WorkerThread,   // Prefer worker threads
    Specific        // Run on specific thread ID
};

// Forward declarations
class Job;
class JobSystem;
class TaskGraph;

using JobHandle = uint64_t;
using JobFunction = std::function<void()>;

// Job status
enum class JobStatus {
    Pending,        // Waiting to be scheduled
    Queued,         // In queue waiting for thread
    Running,        // Currently executing
    Completed,      // Finished successfully
    Failed,         // Finished with error
    Cancelled       // Cancelled before completion
};

// Job descriptor with all configuration
struct JobDescriptor {
    std::string name;
    JobFunction function;
    TaskPriority priority;
    TaskAffinity affinity;
    size_t specificThreadId;
    std::vector<JobHandle> dependencies;
    bool canBeCancelled;
    std::chrono::milliseconds timeout;
    
    JobDescriptor()
        : priority(TaskPriority::Normal)
        , affinity(TaskAffinity::Any)
        , specificThreadId(0)
        , canBeCancelled(true)
        , timeout(std::chrono::milliseconds::zero())
    {}
};

// Job class representing a unit of work
class Job {
    friend class JobSystem;
    
private:
    JobHandle handle;
    JobDescriptor descriptor;
    std::atomic<JobStatus> status;
    std::atomic<int> dependencyCount;
    std::vector<JobHandle> dependents;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    std::exception_ptr exception;
    std::mutex mutex;
    std::condition_variable completionCV;
    
public:
    Job(JobHandle h, const JobDescriptor& desc)
        : handle(h)
        , descriptor(desc)
        , status(JobStatus::Pending)
        , dependencyCount(static_cast<int>(desc.dependencies.size()))
    {}
    
    JobHandle getHandle() const { return handle; }
    JobStatus getStatus() const { return status.load(); }
    const std::string& getName() const { return descriptor.name; }
    TaskPriority getPriority() const { return descriptor.priority; }
    
    bool isComplete() const {
        auto s = status.load();
        return s == JobStatus::Completed || s == JobStatus::Failed || s == JobStatus::Cancelled;
    }
    
    void wait() {
        std::unique_lock<std::mutex> lock(mutex);
        completionCV.wait(lock, [this] { return isComplete(); });
    }
    
    bool waitFor(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex);
        return completionCV.wait_for(lock, timeout, [this] { return isComplete(); });
    }
    
    std::chrono::microseconds getExecutionTime() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    }
    
    void rethrowException() {
        if (exception) {
            std::rethrow_exception(exception);
        }
    }
};

// Priority-based job queue
class PriorityJobQueue {
private:
    static constexpr size_t NUM_PRIORITIES = static_cast<size_t>(TaskPriority::Count);
    std::array<std::deque<std::shared_ptr<Job>>, NUM_PRIORITIES> queues;
    mutable std::mutex mutex;
    std::condition_variable condition;
    std::atomic<size_t> totalCount;
    
public:
    PriorityJobQueue() : totalCount(0) {}
    
    void push(std::shared_ptr<Job> job) {
        std::lock_guard<std::mutex> lock(mutex);
        size_t priority = static_cast<size_t>(job->getPriority());
        queues[priority].push_back(std::move(job));
        ++totalCount;
        condition.notify_one();
    }
    
    std::shared_ptr<Job> pop() {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [this] { return totalCount > 0; });
        
        for (auto& queue : queues) {
            if (!queue.empty()) {
                auto job = std::move(queue.front());
                queue.pop_front();
                --totalCount;
                return job;
            }
        }
        return nullptr;
    }
    
    std::shared_ptr<Job> tryPop() {
        std::lock_guard<std::mutex> lock(mutex);
        
        for (auto& queue : queues) {
            if (!queue.empty()) {
                auto job = std::move(queue.front());
                queue.pop_front();
                --totalCount;
                return job;
            }
        }
        return nullptr;
    }
    
    bool trySteal(std::shared_ptr<Job>& outJob, TaskPriority maxPriority = TaskPriority::Idle) {
        std::lock_guard<std::mutex> lock(mutex);
        
        // Steal from back (oldest jobs of same priority)
        for (size_t i = 0; i <= static_cast<size_t>(maxPriority); ++i) {
            if (!queues[i].empty()) {
                outJob = std::move(queues[i].back());
                queues[i].pop_back();
                --totalCount;
                return true;
            }
        }
        return false;
    }
    
    size_t size() const { return totalCount.load(); }
    bool empty() const { return totalCount.load() == 0; }
    
    void notifyAll() { condition.notify_all(); }
};

// Per-thread local queue for work stealing
class ThreadLocalQueue {
private:
    std::deque<std::shared_ptr<Job>> localQueue;
    mutable std::mutex mutex;
    
public:
    void push(std::shared_ptr<Job> job) {
        std::lock_guard<std::mutex> lock(mutex);
        localQueue.push_front(std::move(job));  // LIFO for local
    }
    
    std::shared_ptr<Job> pop() {
        std::lock_guard<std::mutex> lock(mutex);
        if (localQueue.empty()) return nullptr;
        auto job = std::move(localQueue.front());
        localQueue.pop_front();
        return job;
    }
    
    std::shared_ptr<Job> steal() {
        std::lock_guard<std::mutex> lock(mutex);
        if (localQueue.empty()) return nullptr;
        auto job = std::move(localQueue.back());  // FIFO for stealing
        localQueue.pop_back();
        return job;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return localQueue.size();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return localQueue.empty();
    }
};

// Job counter for batching
class JobCounter {
private:
    std::atomic<int> count;
    std::mutex mutex;
    std::condition_variable cv;
    
public:
    JobCounter(int initial = 0) : count(initial) {}
    
    void increment() { ++count; }
    
    void decrement() {
        if (--count == 0) {
            std::lock_guard<std::mutex> lock(mutex);
            cv.notify_all();
        }
    }
    
    void wait() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return count.load() == 0; });
    }
    
    bool waitFor(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, timeout, [this] { return count.load() == 0; });
    }
    
    int get() const { return count.load(); }
};

// Task graph node for complex dependencies
struct TaskGraphNode {
    JobHandle handle;
    std::string name;
    JobFunction function;
    TaskPriority priority;
    std::vector<size_t> dependencyIndices;
    std::vector<size_t> dependentIndices;
    
    TaskGraphNode()
        : handle(0)
        , priority(TaskPriority::Normal)
    {}
};

// Task graph for expressing complex job dependencies
class TaskGraph {
private:
    std::vector<TaskGraphNode> nodes;
    std::unordered_map<std::string, size_t> nodeMap;
    bool compiled;
    
public:
    TaskGraph() : compiled(false) {}
    
    size_t addNode(const std::string& name, JobFunction func, TaskPriority priority = TaskPriority::Normal) {
        size_t index = nodes.size();
        TaskGraphNode node;
        node.name = name;
        node.function = std::move(func);
        node.priority = priority;
        nodes.push_back(std::move(node));
        nodeMap[name] = index;
        compiled = false;
        return index;
    }
    
    void addDependency(size_t nodeIndex, size_t dependencyIndex) {
        if (nodeIndex < nodes.size() && dependencyIndex < nodes.size()) {
            nodes[nodeIndex].dependencyIndices.push_back(dependencyIndex);
            nodes[dependencyIndex].dependentIndices.push_back(nodeIndex);
            compiled = false;
        }
    }
    
    void addDependency(const std::string& nodeName, const std::string& dependencyName) {
        auto nodeIt = nodeMap.find(nodeName);
        auto depIt = nodeMap.find(dependencyName);
        if (nodeIt != nodeMap.end() && depIt != nodeMap.end()) {
            addDependency(nodeIt->second, depIt->second);
        }
    }
    
    // Topological sort to verify no cycles
    bool compile() {
        std::vector<int> inDegree(nodes.size(), 0);
        for (const auto& node : nodes) {
            for (size_t dep : node.dependencyIndices) {
                ++inDegree[dep];
            }
        }
        
        std::queue<size_t> queue;
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (inDegree[i] == 0) {
                queue.push(i);
            }
        }
        
        size_t processed = 0;
        while (!queue.empty()) {
            size_t current = queue.front();
            queue.pop();
            ++processed;
            
            for (size_t dep : nodes[current].dependentIndices) {
                if (--inDegree[dep] == 0) {
                    queue.push(dep);
                }
            }
        }
        
        compiled = (processed == nodes.size());
        return compiled;
    }
    
    bool isCompiled() const { return compiled; }
    const std::vector<TaskGraphNode>& getNodes() const { return nodes; }
    
    // Get root nodes (no dependencies)
    std::vector<size_t> getRootNodes() const {
        std::vector<size_t> roots;
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].dependencyIndices.empty()) {
                roots.push_back(i);
            }
        }
        return roots;
    }
};

// Thread worker data
struct WorkerThread {
    std::thread thread;
    std::unique_ptr<ThreadLocalQueue> localQueue;
    std::atomic<bool> running;
    size_t threadIndex;
    std::string name;
    
    // Performance metrics
    std::atomic<uint64_t> jobsExecuted;
    std::atomic<uint64_t> jobsStolen;
    std::atomic<uint64_t> totalExecutionTimeMicros;
    std::atomic<uint64_t> idleTimeMicros;
    std::atomic<uint64_t> stealAttempts;
    std::atomic<uint64_t> successfulSteals;
    
    WorkerThread() 
        : running(false)
        , threadIndex(0)
        , jobsExecuted(0)
        , jobsStolen(0)
        , totalExecutionTimeMicros(0)
        , idleTimeMicros(0)
        , stealAttempts(0)
        , successfulSteals(0)
    {}
    
    // Get thread efficiency (execution time / total time)
    double getEfficiency() const {
        uint64_t total = totalExecutionTimeMicros.load();
        uint64_t idle = idleTimeMicros.load();
        if (total + idle == 0) return 0.0;
        return static_cast<double>(total) / (total + idle);
    }
    
    // Average job execution time
    double getAverageJobTime() const {
        uint64_t executed = jobsExecuted.load();
        if (executed == 0) return 0.0;
        return static_cast<double>(totalExecutionTimeMicros.load()) / executed;
    }
};

// Main job system
class JobSystem {
private:
    std::vector<std::unique_ptr<WorkerThread>> workers;
    PriorityJobQueue globalQueue;
    
    std::unordered_map<JobHandle, std::shared_ptr<Job>> jobs;
    mutable std::mutex jobsMutex;
    
    std::atomic<JobHandle> nextHandle;
    std::atomic<bool> running;
    std::atomic<size_t> activeJobs;
    
    std::mutex waitMutex;
    std::condition_variable waitCV;
    
    size_t mainThreadIndex;
    
    // Thread-local storage
    static thread_local size_t currentThreadIndex;
    static thread_local bool isWorkerThread;
    
public:
    explicit JobSystem(size_t numWorkers = 0)
        : nextHandle(1)
        , running(false)
        , activeJobs(0)
        , mainThreadIndex(0)
    {
        if (numWorkers == 0) {
            numWorkers = std::max(1u, std::thread::hardware_concurrency() - 1);
        }
        
        workers.resize(numWorkers);
        for (size_t i = 0; i < numWorkers; ++i) {
            workers[i] = std::make_unique<WorkerThread>();
            workers[i]->localQueue = std::make_unique<ThreadLocalQueue>();
            workers[i]->threadIndex = i;
            workers[i]->name = "Worker_" + std::to_string(i);
        }
    }
    
    ~JobSystem() {
        shutdown();
    }
    
    void startup() {
        running = true;
        
        for (size_t i = 0; i < workers.size(); ++i) {
            workers[i]->running = true;
            workers[i]->thread = std::thread(&JobSystem::workerMain, this, i);
        }
    }
    
    void shutdown() {
        running = false;
        globalQueue.notifyAll();
        
        for (auto& worker : workers) {
            worker->running = false;
            if (worker->thread.joinable()) {
                worker->thread.join();
            }
        }
    }
    
    // Submit a job
    JobHandle submit(const JobDescriptor& desc) {
        JobHandle handle = nextHandle++;
        auto job = std::make_shared<Job>(handle, desc);
        
        {
            std::lock_guard<std::mutex> lock(jobsMutex);
            jobs[handle] = job;
        }
        
        // Check if all dependencies are satisfied
        if (job->dependencyCount.load() == 0) {
            scheduleJob(job);
        }
        
        return handle;
    }
    
    // Submit with lambda
    template<typename F, typename... Args>
    JobHandle submit(F&& func, Args&&... args) {
        JobDescriptor desc;
        desc.function = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
        return submit(desc);
    }
    
    // Submit with priority
    template<typename F>
    JobHandle submitWithPriority(F&& func, TaskPriority priority) {
        JobDescriptor desc;
        desc.function = std::forward<F>(func);
        desc.priority = priority;
        return submit(desc);
    }
    
    // Submit batch of jobs
    std::vector<JobHandle> submitBatch(const std::vector<JobDescriptor>& descriptors) {
        std::vector<JobHandle> handles;
        handles.reserve(descriptors.size());
        
        for (const auto& desc : descriptors) {
            handles.push_back(submit(desc));
        }
        
        return handles;
    }
    
    // Execute task graph
    std::vector<JobHandle> execute(TaskGraph& graph) {
        if (!graph.isCompiled() && !graph.compile()) {
            return {};
        }
        
        std::vector<JobHandle> handles;
        const auto& nodes = graph.getNodes();
        handles.resize(nodes.size());
        
        // Create jobs for all nodes
        for (size_t i = 0; i < nodes.size(); ++i) {
            JobDescriptor desc;
            desc.name = nodes[i].name;
            desc.function = nodes[i].function;
            desc.priority = nodes[i].priority;
            
            // Convert dependency indices to handles
            for (size_t depIdx : nodes[i].dependencyIndices) {
                if (depIdx < handles.size() && handles[depIdx] != 0) {
                    desc.dependencies.push_back(handles[depIdx]);
                }
            }
            
            handles[i] = submit(desc);
        }
        
        return handles;
    }
    
    // Wait for specific job
    void wait(JobHandle handle) {
        std::shared_ptr<Job> job;
        {
            std::lock_guard<std::mutex> lock(jobsMutex);
            auto it = jobs.find(handle);
            if (it == jobs.end()) return;
            job = it->second;
        }
        
        // Help process jobs while waiting
        while (!job->isComplete()) {
            if (!processOneJob()) {
                std::this_thread::yield();
            }
        }
    }
    
    // Wait for multiple jobs
    void waitAll(const std::vector<JobHandle>& handles) {
        for (JobHandle h : handles) {
            wait(h);
        }
    }
    
    // Wait for all jobs to complete
    void waitIdle() {
        while (activeJobs.load() > 0 || !globalQueue.empty()) {
            if (!processOneJob()) {
                std::this_thread::yield();
            }
        }
    }
    
    // Process jobs on calling thread
    bool processOneJob() {
        auto job = tryGetJob();
        if (job) {
            executeJob(job);
            return true;
        }
        return false;
    }
    
    // Cancel a job
    bool cancel(JobHandle handle) {
        std::lock_guard<std::mutex> lock(jobsMutex);
        auto it = jobs.find(handle);
        if (it != jobs.end() && it->second->descriptor.canBeCancelled) {
            auto expected = JobStatus::Pending;
            if (it->second->status.compare_exchange_strong(expected, JobStatus::Cancelled)) {
                it->second->completionCV.notify_all();
                return true;
            }
        }
        return false;
    }
    
    // Get job status
    JobStatus getJobStatus(JobHandle handle) const {
        std::lock_guard<std::mutex> lock(jobsMutex);
        auto it = jobs.find(handle);
        if (it != jobs.end()) {
            return it->second->getStatus();
        }
        return JobStatus::Completed;  // Assume completed if not found
    }
    
    // Performance metrics
    struct WorkerStats {
        size_t threadIndex;
        std::string name;
        uint64_t jobsExecuted;
        uint64_t jobsStolen;
        uint64_t totalExecutionTimeMicros;
        uint64_t idleTimeMicros;
        uint64_t stealAttempts;
        uint64_t successfulSteals;
        size_t localQueueSize;
        double efficiency;
        double avgJobTimeMicros;
    };
    
    std::vector<WorkerStats> getWorkerStats() const {
        std::vector<WorkerStats> stats;
        for (const auto& worker : workers) {
            WorkerStats ws;
            ws.threadIndex = worker->threadIndex;
            ws.name = worker->name;
            ws.jobsExecuted = worker->jobsExecuted.load();
            ws.jobsStolen = worker->jobsStolen.load();
            ws.totalExecutionTimeMicros = worker->totalExecutionTimeMicros.load();
            ws.idleTimeMicros = worker->idleTimeMicros.load();
            ws.stealAttempts = worker->stealAttempts.load();
            ws.successfulSteals = worker->successfulSteals.load();
            ws.localQueueSize = worker->localQueue->size();
            ws.efficiency = worker->getEfficiency();
            ws.avgJobTimeMicros = worker->getAverageJobTime();
            stats.push_back(ws);
        }
        return stats;
    }
    
    // Get aggregated system metrics
    struct SystemMetrics {
        uint64_t totalJobsExecuted;
        uint64_t totalJobsStolen;
        uint64_t totalExecutionTimeMicros;
        uint64_t totalIdleTimeMicros;
        double averageEfficiency;
        double stealSuccessRate;
        size_t activeJobs;
        size_t queuedJobs;
    };
    
    SystemMetrics getSystemMetrics() const {
        SystemMetrics metrics{};
        
        for (const auto& worker : workers) {
            metrics.totalJobsExecuted += worker->jobsExecuted.load();
            metrics.totalJobsStolen += worker->jobsStolen.load();
            metrics.totalExecutionTimeMicros += worker->totalExecutionTimeMicros.load();
            metrics.totalIdleTimeMicros += worker->idleTimeMicros.load();
            metrics.averageEfficiency += worker->getEfficiency();
            
            uint64_t attempts = worker->stealAttempts.load();
            uint64_t successes = worker->successfulSteals.load();
            if (attempts > 0) {
                metrics.stealSuccessRate += static_cast<double>(successes) / attempts;
            }
        }
        
        if (!workers.empty()) {
            metrics.averageEfficiency /= workers.size();
            metrics.stealSuccessRate /= workers.size();
        }
        
        metrics.activeJobs = activeJobs.load();
        metrics.queuedJobs = globalQueue.size();
        
        return metrics;
    }
    
private:
    void workerMain(size_t threadIndex) {
        currentThreadIndex = threadIndex;
        isWorkerThread = true;
        
        auto& worker = *workers[threadIndex];
        
        while (worker.running.load()) {
            // Try local queue first
            auto job = worker.localQueue->pop();
            
            // Try global queue
            if (!job) {
                job = globalQueue.tryPop();
            }
            
            // Try stealing from other workers
            if (!job) {
                job = tryStealJob(threadIndex);
                if (job) {
                    ++worker.jobsStolen;
                }
            }
            
            if (job) {
                executeJob(job);
                ++worker.jobsExecuted;
            } else {
                // Wait for work
                job = globalQueue.pop();
                if (job) {
                    executeJob(job);
                    ++worker.jobsExecuted;
                }
            }
        }
    }
    
    void scheduleJob(std::shared_ptr<Job> job) {
        job->status = JobStatus::Queued;
        
        if (isWorkerThread && currentThreadIndex < workers.size()) {
            // Push to local queue for better cache locality
            workers[currentThreadIndex]->localQueue->push(job);
        } else {
            globalQueue.push(job);
        }
    }
    
    void executeJob(std::shared_ptr<Job> job) {
        ++activeJobs;
        
        job->status = JobStatus::Running;
        job->startTime = std::chrono::high_resolution_clock::now();
        
        try {
            job->descriptor.function();
            job->status = JobStatus::Completed;
        } catch (...) {
            job->exception = std::current_exception();
            job->status = JobStatus::Failed;
        }
        
        job->endTime = std::chrono::high_resolution_clock::now();
        
        // Track execution time
        if (isWorkerThread && currentThreadIndex < workers.size()) {
            workers[currentThreadIndex]->totalExecutionTimeMicros += 
                job->getExecutionTime().count();
        }
        
        // Notify completion
        {
            std::lock_guard<std::mutex> lock(job->mutex);
            job->completionCV.notify_all();
        }
        
        // Signal dependents
        signalDependents(job->handle);
        
        --activeJobs;
        
        {
            std::lock_guard<std::mutex> lock(waitMutex);
            waitCV.notify_all();
        }
    }
    
    void signalDependents(JobHandle handle) {
        std::shared_ptr<Job> completedJob;
        {
            std::lock_guard<std::mutex> lock(jobsMutex);
            auto it = jobs.find(handle);
            if (it == jobs.end()) return;
            completedJob = it->second;
        }
        
        for (JobHandle depHandle : completedJob->dependents) {
            std::shared_ptr<Job> dependent;
            {
                std::lock_guard<std::mutex> lock(jobsMutex);
                auto it = jobs.find(depHandle);
                if (it == jobs.end()) continue;
                dependent = it->second;
            }
            
            if (--dependent->dependencyCount == 0) {
                scheduleJob(dependent);
            }
        }
    }
    
    std::shared_ptr<Job> tryGetJob() {
        if (isWorkerThread && currentThreadIndex < workers.size()) {
            auto job = workers[currentThreadIndex]->localQueue->pop();
            if (job) return job;
        }
        
        return globalQueue.tryPop();
    }
    
    std::shared_ptr<Job> tryStealJob(size_t thiefIndex) {
        size_t numWorkers = workers.size();
        size_t startIndex = (thiefIndex + 1) % numWorkers;
        
        for (size_t i = 0; i < numWorkers; ++i) {
            size_t victimIndex = (startIndex + i) % numWorkers;
            if (victimIndex != thiefIndex) {
                auto job = workers[victimIndex]->localQueue->steal();
                if (job) return job;
            }
        }
        
        return nullptr;
    }
};

// Static member initialization
inline thread_local size_t JobSystem::currentThreadIndex = 0;
inline thread_local bool JobSystem::isWorkerThread = false;

// Parallel for helper
class ParallelFor {
public:
    static void execute(JobSystem& jobSystem, size_t count, size_t batchSize, 
                       std::function<void(size_t, size_t)> func, TaskPriority priority = TaskPriority::Normal) {
        if (count == 0) return;
        
        size_t numBatches = (count + batchSize - 1) / batchSize;
        std::vector<JobHandle> handles;
        handles.reserve(numBatches);
        
        for (size_t batch = 0; batch < numBatches; ++batch) {
            size_t start = batch * batchSize;
            size_t end = std::min(start + batchSize, count);
            
            JobDescriptor desc;
            desc.priority = priority;
            desc.function = [func, start, end]() {
                func(start, end);
            };
            
            handles.push_back(jobSystem.submit(desc));
        }
        
        jobSystem.waitAll(handles);
    }
    
    template<typename Iterator, typename Func>
    static void forEach(JobSystem& jobSystem, Iterator begin, Iterator end, Func func,
                       size_t batchSize = 64, TaskPriority priority = TaskPriority::Normal) {
        size_t count = std::distance(begin, end);
        
        execute(jobSystem, count, batchSize, [begin, &func](size_t start, size_t stop) {
            auto it = begin;
            std::advance(it, start);
            for (size_t i = start; i < stop; ++i, ++it) {
                func(*it);
            }
        }, priority);
    }
};

// Scoped job waiter
class ScopedJobWait {
private:
    JobSystem& system;
    std::vector<JobHandle> handles;
    
public:
    explicit ScopedJobWait(JobSystem& sys) : system(sys) {}
    
    ~ScopedJobWait() {
        system.waitAll(handles);
    }
    
    void add(JobHandle handle) {
        handles.push_back(handle);
    }
    
    template<typename Container>
    void add(const Container& handleContainer) {
        for (auto h : handleContainer) {
            handles.push_back(h);
        }
    }
};

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    
    void wait();
    void stop();
    
    size_t getThreadCount() const { return workers.size(); }
    size_t getQueuedTaskCount();
    size_t getActiveTaskCount() const { return activeTasks.load(); }
    bool isRunning() const { return !stopFlag.load(); }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable waitCondition;
    
    std::atomic<bool> stopFlag;
    std::atomic<size_t> activeTasks;
    
    void workerThread();
};

template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        if (stopFlag) {
            throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
        }
        
        tasks.emplace([task]() { (*task)(); });
    }
    
    condition.notify_one();
    return result;
}

class WorkStealingThreadPool {
public:
    explicit WorkStealingThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~WorkStealingThreadPool();
    
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    
    void wait();
    void stop();
    
    size_t getThreadCount() const { return workers.size(); }

private:
    struct WorkQueue {
        std::queue<std::function<void()>> tasks;
        std::mutex mutex;
    };
    
    std::vector<std::thread> workers;
    std::vector<std::unique_ptr<WorkQueue>> queues;
    
    std::atomic<bool> stopFlag;
    std::atomic<size_t> activeTasks;
    std::condition_variable waitCondition;
    std::mutex waitMutex;
    
    thread_local static size_t threadIndex;
    
    void workerThread(size_t index);
    bool tryStealTask(size_t thiefIndex);
};

template<typename F, typename... Args>
auto WorkStealingThreadPool::submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    size_t index = threadIndex % queues.size();
    
    {
        std::lock_guard<std::mutex> lock(queues[index]->mutex);
        queues[index]->tasks.emplace([task]() { (*task)(); });
    }
    
    return result;
}

} // namespace Threading
} // namespace JJM
