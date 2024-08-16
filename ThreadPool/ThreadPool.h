#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <chrono>
#include <memory>

class ThreadPool {
public:
    ThreadPool(size_t minSize, size_t maxSize);
    ~ThreadPool();

    template<typename Func, typename... Args>
    auto enqueue(Func&& func, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type>;

    void setMaxIdleTime(std::chrono::milliseconds idleTime);

private:
    struct Task {
        std::function<void()> func;
        int priority;

        Task(std::function<void()> f, int p) : func(std::move(f)), priority(p) {}
    };

    struct CompareTask {
        bool operator()(std::shared_ptr<Task> const& t1, std::shared_ptr<Task> const& t2) {
            return t1->priority < t2->priority;
        }
    };

    std::vector<std::thread> workers;
    std::priority_queue<std::shared_ptr<Task>, std::vector<std::shared_ptr<Task>>, CompareTask> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

    size_t minSize;
    size_t maxSize;
    std::atomic<size_t> idleThreads;
    std::chrono::milliseconds maxIdleTime;

    void workerThread();
    void adjustThreadPoolSize();
};

#endif // THREADPOOL_H