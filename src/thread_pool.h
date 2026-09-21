#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <atomic>
#include <future>
#include <cstdint>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) : stop_(false), active_(0) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable()) w.join();
        }
    }

    template<typename F>
    void submit(F&& func) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::forward<F>(func));
        }
        cv_.notify_one();
    }

    void wait_all() {
        std::unique_lock<std::mutex> lock(done_mutex_);
        done_cv_.wait(lock, [this] {
            return tasks_.empty() && active_ == 0;
        });
    }

    size_t thread_count() const { return workers_.size(); }

    static unsigned int optimal_threads() {
        unsigned int hw = std::thread::hardware_concurrency();
        return hw > 0 ? hw : 4;
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
                ++active_;
            }
            task();
            {
                std::lock_guard<std::mutex> lock(done_mutex_);
                --active_;
            }
            done_cv_.notify_one();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex queue_mutex_;
    std::condition_variable cv_;

    std::mutex done_mutex_;
    std::condition_variable done_cv_;
    int active_;
    bool stop_;
};

struct alignas(64) ThreadLocalBuffer {
    std::unordered_map<std::string, std::vector<Event>> match_events;
};
