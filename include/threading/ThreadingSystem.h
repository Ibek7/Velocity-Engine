#ifndef THREADING_SYSTEM_H
#define THREADING_SYSTEM_H

#include <thread>
#include <vector>
#include <queue>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <functional>
#include <memory>
#include <chrono>
#include <unordered_map>

namespace JJM {
namespace Threading {

enum class JobPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

enum class JobState {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled
};

class Job {
private:
    static std::atomic<uint64_t> nextJobID;
    
protected:
    uint64_t jobID;
    std::string name;
    JobPriority priority;
    std::atomic<JobState> state;
    std::function<void()> task;
    std::vector<std::shared_ptr<Job>> dependencies;
    std::atomic<size_t> completedDependencies;
    std::chrono::high_resolution_clock::time_point submitTime;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    
    mutable std::mutex dependencyMutex;
    
public:
    Job(const std::string& name, std::function<void()> task, JobPriority priority = JobPriority::Normal);
    virtual ~Job() = default;
    
    uint64_t getID() const { return jobID; }
    const std::string& getName() const { return name; }
    JobPriority getPriority() const { return priority; }
    JobState getState() const { return state; }
    
    void addDependency(std::shared_ptr<Job> dependency);
    bool areDependenciesCompleted() const;
    void onDependencyCompleted();
    
    virtual void execute();
    void setState(JobState newState);
    
    std::chrono::duration<float> getExecutionTime() const;
    std::chrono::duration<float> getWaitTime() const;
    
    bool operator<(const Job& other) const {
        return priority < other.priority;
    }
};

template<typename T>
class JobWithResult : public Job {
private:
    std::promise<T> resultPromise;
    std::function<T()> resultTask;
    
public:
    JobWithResult(const std::string& name, std::function<T()> task, JobPriority priority = JobPriority::Normal)
        : Job(name, [this]() { 
            try {
                T result = resultTask();
                resultPromise.set_value(result);
            } catch (...) {
                resultPromise.set_exception(std::current_exception());
            }
          }, priority), resultTask(task) {}
    
    std::future<T> getFuture() {
        return resultPromise.get_future();
    }
};

class JobBatch {
private:
    std::vector<std::shared_ptr<Job>> jobs;
    std::atomic<size_t> completedJobs;
    std::promise<void> completionPromise;
    std::string batchName;
    
public:
    JobBatch(const std::string& name);
    
    void addJob(std::shared_ptr<Job> job);
    void addJobs(const std::vector<std::shared_ptr<Job>>& jobList);
    
    std::future<void> getFuture();
    void onJobCompleted();
    
    const std::string& getName() const { return batchName; }
    size_t getJobCount() const { return jobs.size(); }
    size_t getCompletedJobCount() const { return completedJobs; }
    float getProgress() const;
};

class WorkerThread {
private:
    std::thread thread;
    std::atomic<bool> running;
    std::atomic<bool> idle;
    uint32_t threadID;
    std::string threadName;
    
    std::deque<std::shared_ptr<Job>> localQueue;
    std::mutex localQueueMutex;
    
    std::atomic<uint64_t> jobsExecuted;
    std::atomic<uint64_t> jobsStolen;
    std::chrono::duration<float> totalExecutionTime;
    
    class ThreadPool* parentPool;
    
public:
    WorkerThread(uint32_t id, const std::string& name, ThreadPool* pool);
    ~WorkerThread();
    
    void start();
    void stop();
    void join();
    
    bool pushJob(std::shared_ptr<Job> job);
    std::shared_ptr<Job> popJob();
    std::shared_ptr<Job> stealJob();
    
    bool isIdle() const { return idle; }
    uint32_t getID() const { return threadID; }
    const std::string& getName() const { return threadName; }
    
    uint64_t getJobsExecuted() const { return jobsExecuted; }
    uint64_t getJobsStolen() const { return jobsStolen; }
    std::chrono::duration<float> getTotalExecutionTime() const { return totalExecutionTime; }
    
private:
    void workerLoop();
    void executeJob(std::shared_ptr<Job> job);
};

class ThreadPool {
private:
    std::vector<std::unique_ptr<WorkerThread>> workers;
    
    std::priority_queue<std::shared_ptr<Job>, std::vector<std::shared_ptr<Job>>, 
                       std::function<bool(const std::shared_ptr<Job>&, const std::shared_ptr<Job>&)>> globalQueue;
    std::mutex globalQueueMutex;
    std::condition_variable globalQueueCV;
    
    std::unordered_map<uint64_t, std::shared_ptr<Job>> activeJobs;
    std::mutex activeJobsMutex;
    
    std::atomic<bool> running;
    std::atomic<bool> shuttingDown;
    
    uint32_t numThreads;
    bool workStealingEnabled;
    
    // Statistics
    std::atomic<uint64_t> totalJobsSubmitted;
    std::atomic<uint64_t> totalJobsCompleted;
    std::atomic<uint64_t> totalJobsFailed;
    
public:
    ThreadPool(uint32_t numThreads = 0, bool enableWorkStealing = true);
    ~ThreadPool();
    
    void initialize();
    void shutdown();
    
    std::shared_ptr<Job> submitJob(const std::string& name, std::function<void()> task, 
                                   JobPriority priority = JobPriority::Normal);
    
    template<typename T>
    std::shared_ptr<JobWithResult<T>> submitJobWithResult(const std::string& name, 
                                                         std::function<T()> task,
                                                         JobPriority priority = JobPriority::Normal) {
        auto job = std::make_shared<JobWithResult<T>>(name, task, priority);
        submitJobInternal(job);
        return job;
    }
    
    void submitJobBatch(std::shared_ptr<JobBatch> batch);
    
    std::shared_ptr<Job> getJob(uint64_t jobID);
    void cancelJob(uint64_t jobID);
    
    void waitForAll();
    void waitForJob(uint64_t jobID);
    void waitForBatch(std::shared_ptr<JobBatch> batch);
    
    bool isJobCompleted(uint64_t jobID) const;
    
    uint32_t getNumThreads() const { return numThreads; }
    uint32_t getActiveJobCount() const;
    uint32_t getQueuedJobCount() const;
    
    void setWorkStealingEnabled(bool enabled) { workStealingEnabled = enabled; }
    bool isWorkStealingEnabled() const { return workStealingEnabled; }
    
    // Statistics
    uint64_t getTotalJobsSubmitted() const { return totalJobsSubmitted; }
    uint64_t getTotalJobsCompleted() const { return totalJobsCompleted; }
    uint64_t getTotalJobsFailed() const { return totalJobsFailed; }
    
    std::string getStatisticsReport() const;
    void printStatistics() const;
    
    // Friend access for workers
    friend class WorkerThread;
    
private:
    void submitJobInternal(std::shared_ptr<Job> job);
    std::shared_ptr<Job> getNextJob(WorkerThread* requestingWorker);
    void onJobCompleted(std::shared_ptr<Job> job);
    void onJobFailed(std::shared_ptr<Job> job);
    
    bool tryWorkStealing(WorkerThread* requestingWorker, std::shared_ptr<Job>& stolenJob);
    void notifyJobCompletion(std::shared_ptr<Job> job);
};

class ParallelFor {
public:
    template<typename IndexType, typename Function>
    static void execute(IndexType start, IndexType end, Function func, 
                       uint32_t numThreads = 0, uint32_t minItemsPerThread = 1) {
        ThreadPool* pool = ThreadManager::getInstance()->getMainThreadPool();
        
        if (numThreads == 0) {
            numThreads = pool->getNumThreads();
        }
        
        IndexType totalItems = end - start;
        IndexType itemsPerThread = std::max(static_cast<IndexType>(minItemsPerThread), 
                                           totalItems / numThreads);
        
        auto batch = std::make_shared<JobBatch>("ParallelFor");
        
        for (IndexType i = start; i < end; i += itemsPerThread) {
            IndexType chunkEnd = std::min(i + itemsPerThread, end);
            
            auto job = std::make_shared<Job>(
                "ParallelForChunk_" + std::to_string(i),
                [func, i, chunkEnd]() {
                    for (IndexType idx = i; idx < chunkEnd; ++idx) {
                        func(idx);
                    }
                }
            );
            
            batch->addJob(job);
        }
        
        pool->submitJobBatch(batch);
        batch->getFuture().wait();
    }
    
    template<typename Container, typename Function>
    static void executeContainer(Container& container, Function func, uint32_t numThreads = 0) {
        execute(0, container.size(), [&container, func](size_t index) {
            func(container[index]);
        }, numThreads);
    }
};

class TaskScheduler {
private:
    ThreadPool* threadPool;
    std::vector<std::shared_ptr<Job>> scheduledJobs;
    std::mutex schedulerMutex;
    
    struct ScheduledTask {
        std::shared_ptr<Job> job;
        std::chrono::high_resolution_clock::time_point executeTime;
        std::chrono::duration<float> interval;
        bool repeating;
    };
    
    std::vector<ScheduledTask> timedTasks;
    std::thread schedulerThread;
    std::atomic<bool> schedulerRunning;
    std::condition_variable schedulerCV;
    
public:
    TaskScheduler(ThreadPool* pool);
    ~TaskScheduler();
    
    void start();
    void stop();
    
    void scheduleJob(std::shared_ptr<Job> job, std::chrono::duration<float> delay);
    void scheduleJobRepeating(std::shared_ptr<Job> job, std::chrono::duration<float> interval);
    
    void cancelScheduledJob(uint64_t jobID);
    
private:
    void schedulerLoop();
};

class ThreadManager {
private:
    static ThreadManager* instance;
    
    std::unique_ptr<ThreadPool> mainThreadPool;
    std::unique_ptr<ThreadPool> backgroundThreadPool;
    std::unique_ptr<ThreadPool> ioThreadPool;
    std::unique_ptr<TaskScheduler> scheduler;
    
    std::mutex managerMutex;
    
    ThreadManager();
    
public:
    static ThreadManager* getInstance();
    ~ThreadManager();
    
    void initialize(uint32_t mainThreads = 0, uint32_t backgroundThreads = 2, uint32_t ioThreads = 2);
    void shutdown();
    
    ThreadPool* getMainThreadPool() { return mainThreadPool.get(); }
    ThreadPool* getBackgroundThreadPool() { return backgroundThreadPool.get(); }
    ThreadPool* getIOThreadPool() { return ioThreadPool.get(); }
    TaskScheduler* getScheduler() { return scheduler.get(); }
    
    // Convenience methods
    std::shared_ptr<Job> submitMainJob(const std::string& name, std::function<void()> task,
                                      JobPriority priority = JobPriority::Normal);
    
    std::shared_ptr<Job> submitBackgroundJob(const std::string& name, std::function<void()> task,
                                            JobPriority priority = JobPriority::Normal);
    
    std::shared_ptr<Job> submitIOJob(const std::string& name, std::function<void()> task,
                                    JobPriority priority = JobPriority::Normal);
    
    template<typename T>
    std::shared_ptr<JobWithResult<T>> submitJobWithResult(const std::string& name, 
                                                         std::function<T()> task,
                                                         JobPriority priority = JobPriority::Normal) {
        return mainThreadPool->submitJobWithResult<T>(name, task, priority);
    }
    
    void waitForAllJobs();
    
    std::string getSystemReport() const;
    void printSystemReport() const;
};

// Thread-safe utilities
template<typename T>
class ThreadSafeQueue {
private:
    mutable std::mutex mutex;
    std::queue<T> queue;
    std::condition_variable cv;
    
public:
    void push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(item);
        cv.notify_one();
    }
    
    bool tryPop(T& item) {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty()) {
            return false;
        }
        item = queue.front();
        queue.pop();
        return true;
    }
    
    bool waitAndPop(T& item, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        std::unique_lock<std::mutex> lock(mutex);
        if (cv.wait_for(lock, timeout, [this] { return !queue.empty(); })) {
            item = queue.front();
            queue.pop();
            return true;
        }
        return false;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }
};

template<typename T>
class ThreadSafeVector {
private:
    mutable std::shared_mutex mutex;
    std::vector<T> vector;
    
public:
    void push_back(const T& item) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        vector.push_back(item);
    }
    
    T at(size_t index) const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return vector.at(index);
    }
    
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return vector.size();
    }
    
    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex);
        vector.clear();
    }
    
    std::vector<T> snapshot() const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return vector;
    }
};

// Atomic operations helper
class AtomicCounter {
private:
    std::atomic<uint64_t> counter;
    
public:
    AtomicCounter(uint64_t initial = 0) : counter(initial) {}
    
    uint64_t increment() { return ++counter; }
    uint64_t decrement() { return --counter; }
    uint64_t get() const { return counter; }
    void set(uint64_t value) { counter = value; }
    
    uint64_t fetchAdd(uint64_t value) { return counter.fetch_add(value); }
    uint64_t fetchSub(uint64_t value) { return counter.fetch_sub(value); }
};

} // namespace Threading
} // namespace JJM

#endif // THREADING_SYSTEM_H