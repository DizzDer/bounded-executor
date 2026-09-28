#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace concurrency {

// Nonblocking admission, FIFO dispatch, drain-on-close. Tasks must not destroy
// their own executor or wait for other tasks on the same saturated executor.
class bounded_executor {
public:
    struct statistics {
        std::size_t queued, active, accepted, completed, rejected;
        bool closed;
    };

    explicit bounded_executor(std::size_t workers, std::size_t capacity)
        : capacity_(capacity) {
        if (workers == 0 || capacity == 0)
            throw std::invalid_argument("workers and capacity must be positive");
        workers_.reserve(workers);
        try {
            for (std::size_t i = 0; i < workers; ++i)
                workers_.emplace_back([this] { run(); });
        } catch (...) {
            close();
            for (auto& worker : workers_) worker.join();
            throw;
        }
    }

    bounded_executor(const bounded_executor&) = delete;
    bounded_executor& operator=(const bounded_executor&) = delete;

    ~bounded_executor() {
        close();
        for (auto& worker : workers_) worker.join();
    }

    // An empty optional means closed/full. An accepted future transports both
    // the result and any task exception. Move-only callables are supported.
    template<class F>
    auto try_submit(F&& function)
        -> std::optional<std::future<std::invoke_result_t<std::decay_t<F>&>>> {
        using result_type = std::invoke_result_t<std::decay_t<F>&>;
        auto task = std::make_shared<std::packaged_task<result_type()>>(
            std::forward<F>(function));
        auto result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_ || queue_.size() >= capacity_) {
                ++rejected_;
                return std::nullopt;
            }
            queue_.emplace_back([task] { (*task)(); });
            ++accepted_;
        }
        available_.notify_one();
        return std::optional<std::future<result_type>>(std::move(result));
    }

    // Idempotent, thread-safe, nonblocking. Already admitted work is drained.
    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        available_.notify_all();
    }

    statistics stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return {queue_.size(), active_, accepted_, completed_, rejected_, closed_};
    }

private:
    void run() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                available_.wait(lock, [this] { return closed_ || !queue_.empty(); });
                if (queue_.empty()) return;
                task = std::move(queue_.front());
                queue_.pop_front();
                ++active_;
            }
            task(); // packaged_task captures exceptions from the user callable.
            {
                std::lock_guard<std::mutex> lock(mutex_);
                --active_;
                ++completed_;
            }
        }
    }

    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable available_;
    std::deque<std::function<void()>> queue_;
    std::vector<std::thread> workers_;
    std::size_t active_ = 0, accepted_ = 0, completed_ = 0, rejected_ = 0;
    bool closed_ = false;
};

} // namespace concurrency
