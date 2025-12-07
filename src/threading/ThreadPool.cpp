#include "threading/ThreadPool.h"

namespace JJM {
namespace Threading {

ThreadPool::ThreadPool(size_t numThreads) : stopFlag(false), activeTasks(0) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] { workerThread(); });
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            
            condition.wait(lock, [this] {
                return stopFlag.load() || !tasks.empty();
            });
            
            if (stopFlag.load() && tasks.empty()) {
                return;
            }
            
            if (!tasks.empty()) {
                task = std::move(tasks.front());
                tasks.pop();
            }
        }
        
        if (task) {
            ++activeTasks;
            task();
            --activeTasks;
            
            if (activeTasks == 0) {
                waitCondition.notify_all();
            }
        }
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queueMutex);
    waitCondition.wait(lock, [this] {
        return tasks.empty() && activeTasks.load() == 0;
    });
}

void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stopFlag.store(true);
    }
    
    condition.notify_all();
    
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

size_t ThreadPool::getQueuedTaskCount() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return tasks.size();
}

thread_local size_t WorkStealingThreadPool::threadIndex = 0;

WorkStealingThreadPool::WorkStealingThreadPool(size_t numThreads) 
    : stopFlag(false), activeTasks(0) {
    
    for (size_t i = 0; i < numThreads; ++i) {
        queues.push_back(std::make_unique<WorkQueue>());
    }
    
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this, i] { workerThread(i); });
    }
}

WorkStealingThreadPool::~WorkStealingThreadPool() {
    stop();
}

void WorkStealingThreadPool::workerThread(size_t index) {
    threadIndex = index;
    
    while (!stopFlag.load()) {
        std::function<void()> task;
        
        {
            std::lock_guard<std::mutex> lock(queues[index]->mutex);
            if (!queues[index]->tasks.empty()) {
                task = std::move(queues[index]->tasks.front());
                queues[index]->tasks.pop();
            }
        }
        
        if (!task && !tryStealTask(index)) {
            std::this_thread::yield();
            continue;
        }
        
        if (task) {
            ++activeTasks;
            task();
            --activeTasks;
            
            if (activeTasks == 0) {
                std::lock_guard<std::mutex> lock(waitMutex);
                waitCondition.notify_all();
            }
        }
    }
}

bool WorkStealingThreadPool::tryStealTask(size_t thiefIndex) {
    for (size_t i = 1; i < queues.size(); ++i) {
        size_t victimIndex = (thiefIndex + i) % queues.size();
        
        std::lock_guard<std::mutex> lock(queues[victimIndex]->mutex);
        if (!queues[victimIndex]->tasks.empty()) {
            std::function<void()> task = std::move(queues[victimIndex]->tasks.front());
            queues[victimIndex]->tasks.pop();
            
            if (task) {
                ++activeTasks;
                task();
                --activeTasks;
                
                if (activeTasks == 0) {
                    std::lock_guard<std::mutex> wlock(waitMutex);
                    waitCondition.notify_all();
                }
                return true;
            }
        }
    }
    
    return false;
}

void WorkStealingThreadPool::wait() {
    std::unique_lock<std::mutex> lock(waitMutex);
    waitCondition.wait(lock, [this] {
        bool allEmpty = true;
        for (const auto& queue : queues) {
            std::lock_guard<std::mutex> qlock(queue->mutex);
            if (!queue->tasks.empty()) {
                allEmpty = false;
                break;
            }
        }
        return allEmpty && activeTasks.load() == 0;
    });
}

void WorkStealingThreadPool::stop() {
    stopFlag.store(true);
    
    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

} // namespace Threading
} // namespace JJM
