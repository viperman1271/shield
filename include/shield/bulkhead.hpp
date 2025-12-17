/* Copyright (c) 2025 Michael Filion
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files(the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and /or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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