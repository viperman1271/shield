#pragma once

#include <shield/all.hpp>

#include <chrono>
#include <iostream>
#include <mutex>
#include <unordered_map>

namespace shield
{
namespace detail
{
    class circuit_breaker;
    class circuit_breaker_manager;
}

class circuit_breaker final
{
public:
    struct config
    {
        config()
            : failureThreshold(5)
            , timeout(std::chrono::seconds(60))
            , name("default")
        {
        }

        int failureThreshold;
        std::chrono::seconds timeout;
        std::string name;
    };

    enum class state
    {
        closed,
        open,
        half_open
    };

    circuit_breaker(const std::string& name, int failureThreshold = 5, std::chrono::seconds timeout = std::chrono::seconds(60));
    circuit_breaker(const config& cfg);
    ~circuit_breaker();

//     template<typename Func>
//     auto execute(Func&& func)
//     {
//         if (state == state::open)
//         {
//             auto now = std::chrono::steady_clock::now();
//             if (now - lastFailureTime > timeout)
//             {
//                 std::cout << "Circuit transitioning to HALF_OPEN\n";
//                 state = state::half_open;
//             }
//             else
//             {
//                 throw std::runtime_error("Circuit breaker is OPEN");
//             }
//         }
// 
//         try
//         {
//             auto result = func();
//             on_success();
//             return result;
//         }
//         catch (...)
//         {
//             on_failure();
//             throw;
//         }
//     }

    state get_state() const;
    int get_failure_count() const;

//private:
    void on_success();
    void on_failure();
    void on_execute_function();

    std::unique_ptr<detail::circuit_breaker> pImpl;
};

class circuit_breaker_manager final
{
public:
    circuit_breaker_manager();
    ~circuit_breaker_manager() = default;

    std::shared_ptr<circuit_breaker> create(const circuit_breaker::config& cfg);
    std::shared_ptr<circuit_breaker> get(const std::string& name);

private:
    std::unique_ptr<detail::circuit_breaker_manager> pImpl;
};

class circuit
{
public:
    circuit(const std::string& name);
    circuit(std::shared_ptr<circuit_breaker> breaker);

    template<class Func, class _Ty, class _Texcept = std::exception>
    void run(Func&& func, retry_policy retry = default_retry_policy, timeout_policy timeout = default_timeout_policy, fallback_policy fallback = default_fallback_policy) const
    {
        on_execute_function();

        using Ret = std::invoke_result_t<Func>;
        if constexpr (std::is_void_v<Ret> || !std::is_convertible_v<Ret, bool>)
        {
            try
            {
                func();
                on_success();
            }
            catch (const _Texcept& ex)
            {
                on_failure();
            }
        }
        else
        {
            try
            {
                Ret result = func();
                if (result)
                {
                    on_success();
                }
                else
                {
                    on_failure();
                }
            }
            catch (const _Texcept& ex)
            {
                on_failure();
            }
        }
    }

private:
    void on_success();
    void on_failure();
    void on_execute_function();

private:
    std::shared_ptr<circuit_breaker> circuitBreaker;
};
} // shield