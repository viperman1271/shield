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

#include <boost/asio.hpp>
//#include <boost/asio/executor_work_guard>

#include <chrono>
#include <future>
#include <thread>

namespace shield
{
template<typename Func>
auto with_timeout(Func&& func, std::chrono::milliseconds timeout)
{
    using return_type = decltype(func());
    
    auto future = std::async(std::launch::async, std::forward<Func>(func));
    
    if (future.wait_for(timeout) == std::future_status::timeout)
    {
        throw std::runtime_error("Operation timed out");
    }
    
    return future.get();
}

// Alternative using Boost.Asio
class timeout_executor final
{
public:
    timeout_executor();
    ~timeout_executor();
    
    template<typename Func>
    auto execute_with_timeout(Func&& func, std::chrono::milliseconds timeout)
    {
        using return_type = decltype(func());
        auto promise = std::make_shared<std::promise<return_type>>();
        auto future = promise->get_future();
        
        boost::asio::steady_timer timer(ioContext, timeout);
        std::atomic<bool> completed{false};
        
        // Execute function in separate thread
        std::thread([func, promise, &completed]()
        {
            try
            {
                if constexpr (std::is_void_v<return_type>)
                {
                    func();
                    promise->set_value();
                }
                else
                {
                    promise->set_value(func());
                }
                completed = true;
            }
            catch (...)
            {
                promise->set_exception(std::current_exception());
                completed = true;
            }
        }).detach();
        
        // Setup timeout
        timer.async_wait([promise, &completed](const boost::system::error_code& ec)
        {
            if (!ec && !completed)
            {
                promise->set_exception(std::make_exception_ptr(std::runtime_error("Timeout")));
            }
        });
        
        return future.get();
    }
    
private:
    boost::asio::io_context ioContext;
    //boost::asio::io_context::executor_work_guard workGuard;

    std::thread thread;
};

struct timeout_policy final
{
    timeout_policy(std::chrono::seconds timeout)
        : timeout(timeout)
    {
    }

    std::chrono::seconds timeout;
};

static const timeout_policy default_timeout_policy = timeout_policy(std::chrono::seconds(1));
} // shield