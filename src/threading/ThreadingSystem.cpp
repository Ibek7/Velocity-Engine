#include "threading/ThreadingSystem.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>

namespace JJM {
namespace Threading {

// Static member initialization
std::atomic<uint64_t> Job::nextJobID(1);
ThreadManager* ThreadManager::instance = nullptr;

// Job implementation
Job::Job(const std::string& name, std::function<void()> task, JobPriority priority)
    : jobID(nextJobID++), name(name), priority(priority), state(JobState::Pending),
      task(task), completedDependencies(0),
      submitTime(std::chrono::high_resolution_clock::now()) {}

void Job::addDependency(std::shared_ptr<Job> dependency) {
    std::lock_guard<std::mutex> lock(dependencyMutex);
    dependencies.push_back(dependency);
}

bool Job::areDependenciesCompleted() const {
    return completedDependencies.load() >= dependencies.size();
}

void Job::onDependencyCompleted() {
    completedDependencies++;
}

void Job::execute() {
    if (state != JobState::Pending) return;
    
    startTime = std::chrono::high_resolution_clock::now();
    setState(JobState::Running);
    
    try {
        if (task) {
            task();
        }
        setState(JobState::Completed);
    } catch (const std::exception& e) {
        std::cerr << "Job " << name << " failed: " << e.what() << std::endl;
        setState(JobState::Failed);
    } catch (...) {
        std::cerr << "Job " << name << " failed with unknown exception" << std::endl;
        setState(JobState::Failed);
    }
    
    endTime = std::chrono::high_resolution_clock::now();
}

void Job::setState(JobState newState) {
    state = newState;
}

std::chrono::duration<float> Job::getExecutionTime() const {
    if (state == JobState::Completed || state == JobState::Failed) {
        return endTime - startTime;
    }
    return std::chrono::duration<float>(0);
}

std::chrono::duration<float> Job::getWaitTime() const {
    if (startTime > submitTime) {
        return startTime - submitTime;
    }
    return std::chrono::duration<float>(0);
}

// JobBatch implementation
JobBatch::JobBatch(const std::string& name) : batchName(name), completedJobs(0) {}

void JobBatch::addJob(std::shared_ptr<Job> job) {
    jobs.push_back(job);
}

void JobBatch::addJobs(const std::vector<std::shared_ptr<Job>>& jobList) {
    jobs.insert(jobs.end(), jobList.begin(), jobList.end());
}

std::future<void> JobBatch::getFuture() {
    return completionPromise.get_future();
}

void JobBatch::onJobCompleted() {
    if (++completedJobs >= jobs.size()) {
        completionPromise.set_value();
    }
}

float JobBatch::getProgress() const {
    if (jobs.empty()) return 1.0f;
    return static_cast<float>(completedJobs.load()) / static_cast<float>(jobs.size());
}

// WorkerThread implementation
WorkerThread::WorkerThread(uint32_t id, const std::string& name, ThreadPool* pool)
    : threadID(id), threadName(name), parentPool(pool),
      running(false), idle(true), jobsExecuted(0), jobsStolen(0),
      totalExecutionTime(0) {}

WorkerThread::~WorkerThread() {
    stop();
    join();
}

void WorkerThread::start() {
    running = true;
    thread = std::thread(&WorkerThread::workerLoop, this);
}

void WorkerThread::stop() {
    running = false;
    // Wake up thread if it's waiting
}

void WorkerThread::join() {
    if (thread.joinable()) {
        thread.join();
    }
}

bool WorkerThread::pushJob(std::shared_ptr<Job> job) {
    std::lock_guard<std::mutex> lock(localQueueMutex);
    localQueue.push_back(job);
    return true;
}

std::shared_ptr<Job> WorkerThread::popJob() {
    std::lock_guard<std::mutex> lock(localQueueMutex);
    
    if (localQueue.empty()) {
        return nullptr;
    }
    
    auto job = localQueue.front();
    localQueue.pop_front();
    return job;
}

std::shared_ptr<Job> WorkerThread::stealJob() {
    std::lock_guard<std::mutex> lock(localQueueMutex);
    
    if (localQueue.empty()) {
        return nullptr;
    }
    
    // Steal from the back (LIFO for better cache locality)
    auto job = localQueue.back();
    localQueue.pop_back();
    jobsStolen++;
    
    return job;
}

void WorkerThread::workerLoop() {
    while (running) {
        std::shared_ptr<Job> job = nullptr;
        
        // Try to get job from local queue first
        job = popJob();
        
        // If no local job, try to get from global queue or steal
        if (!job) {
            job = parentPool->getNextJob(this);
        }
        
        if (job) {
            idle = false;
            executeJob(job);
            idle = true;
            jobsExecuted++;
        } else {
            // No work available, sleep briefly
            idle = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void WorkerThread::executeJob(std::shared_ptr<Job> job) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    job->execute();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    totalExecutionTime += endTime - startTime;
    
    // Notify pool of completion
    parentPool->onJobCompleted(job);
}

// ThreadPool implementation
ThreadPool::ThreadPool(uint32_t numThreads, bool enableWorkStealing)
    : running(false), shuttingDown(false), workStealingEnabled(enableWorkStealing),
      totalJobsSubmitted(0), totalJobsCompleted(0), totalJobsFailed(0),
      globalQueue([](const std::shared_ptr<Job>& a, const std::shared_ptr<Job>& b) {
          return a->getPriority() < b->getPriority(); // Higher priority first
      }) {
    
    if (numThreads == 0) {
        this->numThreads = std::thread::hardware_concurrency();
    } else {
        this->numThreads = numThreads;
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::initialize() {
    if (running) return;
    
    running = true;
    shuttingDown = false;
    
    workers.reserve(numThreads);
    for (uint32_t i = 0; i < numThreads; ++i) {
        std::string workerName = "Worker_" + std::to_string(i);
        auto worker = std::make_unique<WorkerThread>(i, workerName, this);
        worker->start();
        workers.push_back(std::move(worker));
    }
    
    std::cout << "ThreadPool initialized with " << numThreads << " threads" << std::endl;
}

void ThreadPool::shutdown() {
    if (!running) return;
    
    shuttingDown = true;
    
    // Wait for all jobs to complete
    waitForAll();
    
    // Stop all workers
    running = false;
    globalQueueCV.notify_all();
    
    for (auto& worker : workers) {
        worker->stop();
        worker->join();
    }
    
    workers.clear();
    
    std::cout << "ThreadPool shutdown complete" << std::endl;
}

std::shared_ptr<Job> ThreadPool::submitJob(const std::string& name, std::function<void()> task, JobPriority priority) {
    auto job = std::make_shared<Job>(name, task, priority);
    submitJobInternal(job);
    return job;
}

void ThreadPool::submitJobBatch(std::shared_ptr<JobBatch> batch) {
    for (size_t i = 0; i < batch->getJobCount(); ++i) {
        // Would submit each job in the batch
        // For now, just increment the submitted counter
        totalJobsSubmitted++;
    }
}

void ThreadPool::submitJobInternal(std::shared_ptr<Job> job) {
    {
        std::lock_guard<std::mutex> lock(globalQueueMutex);
        globalQueue.push(job);
        
        std::lock_guard<std::mutex> activeLock(activeJobsMutex);
        activeJobs[job->getID()] = job;
    }
    
    totalJobsSubmitted++;
    globalQueueCV.notify_one();
}

std::shared_ptr<Job> ThreadPool::getNextJob(WorkerThread* requestingWorker) {
    // First, try global queue
    {
        std::unique_lock<std::mutex> lock(globalQueueMutex);
        
        if (globalQueueCV.wait_for(lock, std::chrono::milliseconds(10), 
                                  [this] { return !globalQueue.empty() || shuttingDown; })) {
            
            if (!globalQueue.empty()) {
                auto job = globalQueue.top();
                globalQueue.pop();
                
                // Check if dependencies are met
                if (job->areDependenciesCompleted()) {
                    return job;
                } else {
                    // Put back in queue (would normally handle dependency resolution)
                    globalQueue.push(job);
                }
            }
        }
    }
    
    // If work stealing is enabled, try to steal from other workers
    if (workStealingEnabled && requestingWorker) {
        std::shared_ptr<Job> stolenJob;
        if (tryWorkStealing(requestingWorker, stolenJob)) {
            return stolenJob;
        }
    }
    
    return nullptr;
}

void ThreadPool::onJobCompleted(std::shared_ptr<Job> job) {
    {
        std::lock_guard<std::mutex> lock(activeJobsMutex);
        activeJobs.erase(job->getID());
    }
    
    totalJobsCompleted++;
    
    // Notify dependent jobs
    notifyJobCompletion(job);
}

void ThreadPool::onJobFailed(std::shared_ptr<Job> job) {
    {
        std::lock_guard<std::mutex> lock(activeJobsMutex);
        activeJobs.erase(job->getID());
    }
    
    totalJobsFailed++;
}

bool ThreadPool::tryWorkStealing(WorkerThread* requestingWorker, std::shared_ptr<Job>& stolenJob) {
    // Try to steal from other worker threads
    for (auto& worker : workers) {
        if (worker.get() != requestingWorker && !worker->isIdle()) {
            stolenJob = worker->stealJob();
            if (stolenJob) {
                return true;
            }
        }
    }
    
    return false;
}

void ThreadPool::notifyJobCompletion(std::shared_ptr<Job> job) {
    // Would notify dependent jobs and batch completion
    // For now, just a placeholder
}

std::shared_ptr<Job> ThreadPool::getJob(uint64_t jobID) {
    std::lock_guard<std::mutex> lock(activeJobsMutex);
    auto it = activeJobs.find(jobID);
    return (it != activeJobs.end()) ? it->second : nullptr;
}

void ThreadPool::cancelJob(uint64_t jobID) {
    auto job = getJob(jobID);
    if (job && job->getState() == JobState::Pending) {
        job->setState(JobState::Cancelled);
    }
}

void ThreadPool::waitForAll() {
    while (getActiveJobCount() > 0 || getQueuedJobCount() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ThreadPool::waitForJob(uint64_t jobID) {
    auto job = getJob(jobID);
    if (job) {
        while (job->getState() == JobState::Pending || job->getState() == JobState::Running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void ThreadPool::waitForBatch(std::shared_ptr<JobBatch> batch) {
    if (batch) {
        batch->getFuture().wait();
    }
}

bool ThreadPool::isJobCompleted(uint64_t jobID) const {
    auto job = const_cast<ThreadPool*>(this)->getJob(jobID);
    return job && (job->getState() == JobState::Completed || 
                   job->getState() == JobState::Failed || 
                   job->getState() == JobState::Cancelled);
}

uint32_t ThreadPool::getActiveJobCount() const {
    std::lock_guard<std::mutex> lock(activeJobsMutex);
    return static_cast<uint32_t>(activeJobs.size());
}

uint32_t ThreadPool::getQueuedJobCount() const {
    std::lock_guard<std::mutex> lock(globalQueueMutex);
    return static_cast<uint32_t>(globalQueue.size());
}

std::string ThreadPool::getStatisticsReport() const {
    std::stringstream ss;
    ss << "=== ThreadPool Statistics ===" << std::endl;
    ss << "Threads: " << numThreads << std::endl;
    ss << "Jobs Submitted: " << totalJobsSubmitted << std::endl;
    ss << "Jobs Completed: " << totalJobsCompleted << std::endl;
    ss << "Jobs Failed: " << totalJobsFailed << std::endl;
    ss << "Active Jobs: " << getActiveJobCount() << std::endl;
    ss << "Queued Jobs: " << getQueuedJobCount() << std::endl;
    ss << "Work Stealing: " << (workStealingEnabled ? "Enabled" : "Disabled") << std::endl;
    
    ss << "\n--- Worker Thread Statistics ---" << std::endl;
    for (const auto& worker : workers) {
        ss << "Worker " << worker->getID() << " (" << worker->getName() << "): ";
        ss << "Jobs Executed: " << worker->getJobsExecuted() << ", ";
        ss << "Jobs Stolen: " << worker->getJobsStolen() << ", ";
        ss << "Idle: " << (worker->isIdle() ? "Yes" : "No") << std::endl;
    }
    
    return ss.str();
}

void ThreadPool::printStatistics() const {
    std::cout << getStatisticsReport() << std::endl;
}

// TaskScheduler implementation
TaskScheduler::TaskScheduler(ThreadPool* pool) 
    : threadPool(pool), schedulerRunning(false) {}

TaskScheduler::~TaskScheduler() {
    stop();
}

void TaskScheduler::start() {
    if (schedulerRunning) return;
    
    schedulerRunning = true;
    schedulerThread = std::thread(&TaskScheduler::schedulerLoop, this);
}

void TaskScheduler::stop() {
    if (!schedulerRunning) return;
    
    schedulerRunning = false;
    schedulerCV.notify_all();
    
    if (schedulerThread.joinable()) {
        schedulerThread.join();
    }
}

void TaskScheduler::scheduleJob(std::shared_ptr<Job> job, std::chrono::duration<float> delay) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    ScheduledTask task;
    task.job = job;
    task.executeTime = std::chrono::high_resolution_clock::now() + delay;
    task.repeating = false;
    
    timedTasks.push_back(task);
    schedulerCV.notify_one();
}

void TaskScheduler::scheduleJobRepeating(std::shared_ptr<Job> job, std::chrono::duration<float> interval) {
    std::lock_guard<std::mutex> lock(schedulerMutex);
    
    ScheduledTask task;
    task.job = job;
    task.executeTime = std::chrono::high_resolution_clock::now() + interval;
    task.interval = interval;
    task.repeating = true;
    
    timedTasks.push_back(task);
    schedulerCV.notify_one();
}

void TaskScheduler::schedulerLoop() {
    while (schedulerRunning) {
        auto now = std::chrono::high_resolution_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(schedulerMutex);
            
            for (auto it = timedTasks.begin(); it != timedTasks.end();) {
                if (now >= it->executeTime) {
                    // Submit job to thread pool
                    threadPool->submitJobInternal(it->job);
                    
                    if (it->repeating) {
                        // Reschedule for next execution
                        it->executeTime = now + it->interval;
                        ++it;
                    } else {
                        // Remove one-time task
                        it = timedTasks.erase(it);
                    }
                } else {
                    ++it;
                }
            }
        }
        
        // Sleep until next check or notification
        std::unique_lock<std::mutex> lock(schedulerMutex);
        schedulerCV.wait_for(lock, std::chrono::milliseconds(100));
    }
}

// ThreadManager implementation
ThreadManager::ThreadManager() {}

ThreadManager* ThreadManager::getInstance() {
    if (!instance) {
        instance = new ThreadManager();
    }
    return instance;
}

ThreadManager::~ThreadManager() {
    shutdown();
}

void ThreadManager::initialize(uint32_t mainThreads, uint32_t backgroundThreads, uint32_t ioThreads) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    if (mainThreads == 0) {
        mainThreads = std::max(1u, std::thread::hardware_concurrency() - 2);
    }
    
    mainThreadPool = std::make_unique<ThreadPool>(mainThreads, true);
    backgroundThreadPool = std::make_unique<ThreadPool>(backgroundThreads, false);
    ioThreadPool = std::make_unique<ThreadPool>(ioThreads, false);
    
    mainThreadPool->initialize();
    backgroundThreadPool->initialize();
    ioThreadPool->initialize();
    
    scheduler = std::make_unique<TaskScheduler>(mainThreadPool.get());
    scheduler->start();
    
    std::cout << "ThreadManager initialized:" << std::endl;
    std::cout << "  Main threads: " << mainThreads << std::endl;
    std::cout << "  Background threads: " << backgroundThreads << std::endl;
    std::cout << "  I/O threads: " << ioThreads << std::endl;
}

void ThreadManager::shutdown() {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    if (scheduler) {
        scheduler->stop();
        scheduler.reset();
    }
    
    if (mainThreadPool) {
        mainThreadPool->shutdown();
        mainThreadPool.reset();
    }
    
    if (backgroundThreadPool) {
        backgroundThreadPool->shutdown();
        backgroundThreadPool.reset();
    }
    
    if (ioThreadPool) {
        ioThreadPool->shutdown();
        ioThreadPool.reset();
    }
    
    std::cout << "ThreadManager shutdown complete" << std::endl;
}

std::shared_ptr<Job> ThreadManager::submitMainJob(const std::string& name, std::function<void()> task, JobPriority priority) {
    return mainThreadPool->submitJob(name, task, priority);
}

std::shared_ptr<Job> ThreadManager::submitBackgroundJob(const std::string& name, std::function<void()> task, JobPriority priority) {
    return backgroundThreadPool->submitJob(name, task, priority);
}

std::shared_ptr<Job> ThreadManager::submitIOJob(const std::string& name, std::function<void()> task, JobPriority priority) {
    return ioThreadPool->submitJob(name, task, priority);
}

void ThreadManager::waitForAllJobs() {
    if (mainThreadPool) mainThreadPool->waitForAll();
    if (backgroundThreadPool) backgroundThreadPool->waitForAll();
    if (ioThreadPool) ioThreadPool->waitForAll();
}

std::string ThreadManager::getSystemReport() const {
    std::stringstream ss;
    ss << "=== Threading System Report ===" << std::endl;
    
    if (mainThreadPool) {
        ss << "\nMain Thread Pool:" << std::endl;
        ss << mainThreadPool->getStatisticsReport() << std::endl;
    }
    
    if (backgroundThreadPool) {
        ss << "\nBackground Thread Pool:" << std::endl;
        ss << backgroundThreadPool->getStatisticsReport() << std::endl;
    }
    
    if (ioThreadPool) {
        ss << "\nI/O Thread Pool:" << std::endl;
        ss << ioThreadPool->getStatisticsReport() << std::endl;
    }
    
    return ss.str();
}

void ThreadManager::printSystemReport() const {
    std::cout << getSystemReport() << std::endl;
}

} // namespace Threading
} // namespace JJM