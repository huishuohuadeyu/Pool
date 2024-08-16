#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t minSize, size_t maxSize)
    : stop(false), minSize(minSize), maxSize(maxSize), idleThreads(0), maxIdleTime(std::chrono::milliseconds(5000)) {
    for (size_t i = 0; i < minSize; ++i) {
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    stop = true;
    condition.notify_all();

    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::setMaxIdleTime(std::chrono::milliseconds idleTime) {
    maxIdleTime = idleTime;
}

void ThreadPool::workerThread() {
    while (!stop) {
        std::shared_ptr<Task> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            if (condition.wait_for(lock, maxIdleTime, [this] { return stop || !tasks.empty(); })) {
                if (stop && tasks.empty()) {
                    return;
                }

                task = tasks.top();
                tasks.pop();
            } else if (workers.size() > minSize) {
                return; // 超过空闲时间并且线程池大小大于最小值时，减少线程
            }
        }

        idleThreads.fetch_sub(1, std::memory_order_relaxed);
        task->func();
        idleThreads.fetch_add(1, std::memory_order_relaxed);

        adjustThreadPoolSize();
    }
}

void ThreadPool::adjustThreadPoolSize() {
    if (idleThreads.load(std::memory_order_relaxed) == 0 && workers.size() < maxSize) {
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

template<typename Func, typename... Args>
auto ThreadPool::enqueue(Func&& func, Args&&... args) -> std::future<typename std::result_of<Func(Args...)>::type> {
    using returnType = typename std::result_of<Func(Args...)>::type;

    auto task = std::make_shared<Task>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...), 0);
    std::future<returnType> result = std::async(std::launch::deferred, [task]() { task->func(); });

    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (stop) {
            throw std::runtime_error("Enqueue on stopped ThreadPool");
        }

        tasks.emplace(task);
    }

    condition.notify_one();
    return result;
}