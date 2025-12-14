#pragma once

#include <folly/executors/ThreadedExecutor.h>
#include <folly/futures/Future.h>

#include <type_traits>

namespace shield
{
class bulkhead
{
public:
    explicit bulkhead(size_t max_concurrent = 10)
        : max_concurrent_(max_concurrent)
        , current_count_(0)
        , executor_(std::make_unique<folly::ThreadedExecutor>())
    {
    }

    template<typename Func>
    folly::Future<typename std::invoke_result<Func()>::type> execute(Func&& func)
    {
        if (current_count_ >= max_concurrent_)
        {
            return folly::makeFuture<typename std::invoke_result<Func()>::type>(
                std::runtime_error("Bulkhead capacity exceeded"));
        }

        current_count_++;

        return folly::via(executor_.get())
            .thenValue([this, f = std::forward<Func>(func)](auto&&) mutable
                {
                    try
                    {
                        auto result = f();
                        current_count_--;
                        return result;
                    }
                    catch (...)
                    {
                        current_count_--;
                        throw;
                    }
                });
    }

    size_t get_current_count() const
    {
        return current_count_.load();
    }

    size_t get_max_concurrent() const
    {
        return max_concurrent_;
    }

private:
    size_t max_concurrent_;
    std::atomic<size_t> current_count_;
    std::unique_ptr<folly::ThreadedExecutor> executor_;
};
}