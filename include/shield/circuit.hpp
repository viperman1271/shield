#pragma once

#include <shield/circuitbreaker.hpp>
#include <shield/exceptions.hpp>
#include <shield/fallback.hpp>
#include <shield/retry.hpp>
#include <shield/timeout.hpp>

#include <itlib/sentry.hpp>

#include <chrono>
#include <iostream>
#include <mutex>
#include <unordered_map>

namespace shield
{
class circuit final
{
public:
    circuit(const std::string& name, retry_policy retry = default_retry_policy, timeout_policy timeout = default_timeout_policy, fallback_policy fallback = default_fallback_policy);
    circuit(std::shared_ptr<circuit_breaker> breaker);

    circuit& with_retry_policy(const retry_policy& policy);

    template<class _Texcept = shield::unused_exception, class Func>
    auto run(Func&& func) const
    {
        using Ret = std::invoke_result_t<Func>;

        if (retryPolicy)
        {
            if constexpr (std::is_void_v<Ret>)
            {
                run_with_retry_policy<_Texcept>(std::forward<Func>(func));
            }
            else
            {
                return run_with_retry_policy<_Texcept>(std::forward<Func>(func));
            }
        }
        else
        {
            if constexpr (std::is_void_v<Ret>)
            {
                run_without_retry_policy<_Texcept>(std::forward<Func>(func));
            }
            else
            {
                return run_without_retry_policy<_Texcept>(std::forward<Func>(func));
            }
        }
    }

    template<class _Texcept = shield::unused_exception, class Func>
    static auto run(Func&& func, const std::string& name, retry_policy retry = default_retry_policy, timeout_policy timeout = default_timeout_policy, fallback_policy fallback = default_fallback_policy)
    {
        circuit cir(name, retry, timeout, fallback);
        return cir.run<_Texcept>(std::forward<Func>(func));
    }

    const retry_policy& get_retry_policy() const;
    const timeout_policy& get_timeout_policy() const;
    const fallback_policy& get_fallback_policy() const;

private:
    template<class _Texcept = shield::unused_exception, class Func>
    auto run_with_retry_policy(Func&& func) const
    {
        using Ret = std::invoke_result_t<Func>;

        if constexpr (std::is_void_v<Ret>)
        {
            retryPolicy->run([this, &func]()
            {
                run_without_retry_policy<_Texcept, true>(std::forward<Func>(func));
            });
        }
        else
        {
            return retryPolicy->run([this, &func]() -> Ret
            {
                return run_without_retry_policy<_Texcept, true>(std::forward<Func>(func));
            });
        }
    }

    template<class _Texcept = shield::unused_exception, bool _AlwaysRethrowExceptions = false, class Func>
    auto run_without_retry_policy(Func&& func) const
    {
        bool succeeded = false;
        itlib::sentry on_exit_function([&]() { handle_function_exit(succeeded); });

        // Check circuit breaker state and throw if open
        if (!on_execute_function())
        {
            throw std::runtime_error("Circuit is OPEN");
        }

        using Ret = std::invoke_result_t<Func>;
        if constexpr (std::is_void_v<Ret>)
        {
            try
            {
                func();
                succeeded = true;
            }
            catch (const _Texcept&)
            {
                succeeded = false;

                if constexpr (_AlwaysRethrowExceptions)
                {
                    throw;
                }
            }
        }
        else
        {
            try
            {
                Ret result = func();
                succeeded = true;
                return result;
            }
            catch (const _Texcept& ex)
            {
                std::cerr << ex.what() << std::endl;
                succeeded = false;

                if constexpr (_AlwaysRethrowExceptions || !std::is_default_constructible_v<Ret>)
                {
                    throw;
                }
                else if constexpr (std::is_default_constructible_v<Ret>)
                {
                    return Ret();
                }
            }
        }
    }

    void on_success() const;
    void on_failure() const;
    bool on_execute_function() const;
    void handle_function_exit(bool success) const;

private:
    std::shared_ptr<circuit_breaker> circuitBreaker;
    std::optional<retry_policy> retryPolicy;
};
} // shield