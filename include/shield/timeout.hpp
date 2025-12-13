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