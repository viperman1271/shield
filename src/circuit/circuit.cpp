#include <shield/circuit.hpp>

#include <detail/circuit/circuitbreakermanager.hpp>

namespace shield
{
circuit::circuit(const std::string& name, retry_policy retry, timeout_policy timeout, fallback_policy fallback)
    : circuitBreaker(detail::circuit_breaker_manager::get_instance().get(name))
{
}

circuit::circuit(std::shared_ptr<circuit_breaker> breaker)
    : circuitBreaker(breaker)
{
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