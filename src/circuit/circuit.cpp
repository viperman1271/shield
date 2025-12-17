#include <shield/circuit.hpp>

#include <detail/circuit/circuitbreakermanager.hpp>

namespace shield
{
circuit::circuit(const std::string& name, std::optional<retry_policy> retry, std::optional<timeout_policy> timeout, std::optional<fallback_policy> fallback)
    : circuitBreaker(detail::circuit_breaker_manager::get_instance().get_or_create(name))
    , retryPolicy(std::move(retry))
    , fallbackPolicy(std::move(fallback))
{
    if (retryPolicy.has_value() && fallbackPolicy.has_value())
    {
        retryPolicy->set_fallback_policy(fallbackPolicy.value());
    }
}

circuit::circuit(std::shared_ptr<circuit_breaker> breaker)
    : circuitBreaker(breaker)
{
}

circuit& circuit::with_retry_policy(const retry_policy& policy)
{
    retryPolicy = policy;
    if (fallbackPolicy.has_value())
    {
        retryPolicy->set_fallback_policy(fallbackPolicy.value());
    }
    return *this;
}

circuit& circuit::with_fallback_policy(const fallback_policy& policy)
{
    fallbackPolicy = policy;
    if (retryPolicy.has_value())
    {
        retryPolicy->set_fallback_policy(policy);
    }
    return *this;
}

void circuit::on_success() const
{
    detail::circuit_breaker_manager::get_instance().on_success(circuitBreaker);
}

void circuit::on_failure() const
{
    detail::circuit_breaker_manager::get_instance().on_failure(circuitBreaker);
}

bool circuit::on_execute_function() const
{
    return detail::circuit_breaker_manager::get_instance().on_execute_function(circuitBreaker);
}

void circuit::handle_function_exit(bool success) const
{
    if (success)
    {
        on_success();
    }
    else
    {
        on_failure();
    }
}
} // shield