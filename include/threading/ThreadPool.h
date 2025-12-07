#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>

namespace JJM {
namespace Threading {

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
