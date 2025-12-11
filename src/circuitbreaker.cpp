#include <shield/circuitbreaker.hpp>

namespace shield
{
circuit_breaker::circuit_breaker(const std::string& name, int failureThreshold, std::chrono::seconds timeout)
    : failureThreshold(failureThreshold)
    , timeout(timeout)
    , name(name)
    , failureCount(0)
    , state(state::closed)
{
}

circuit_breaker::circuit_breaker(const config& cfg)
    : failureThreshold(cfg.failureThreshold)
    , name(cfg.name)
    , timeout(cfg.timeout)
    , failureCount(0)
    , state(state::closed)
{
}

void circuit_breaker::on_success()
{
    std::lock_guard<std::mutex> lock(mutex);
    failureCount = 0;
    if (state == state::half_open)
    {
        std::cout << "[cb] Transitioning '" << name << "' from HALF_OPEN to CLOSED" << std::endl;
        state = state::closed;
    }
}

void circuit_breaker::on_failure()
{
    std::lock_guard<std::mutex> lock(mutex);
    ++failureCount;
    lastFailureTime = std::chrono::steady_clock::now();

    if (failureCount >= failureThreshold && state != state::open)
    {
        std::cout << "[cb] Transitioning '" << name << "' OPEN" << std::endl;
        state = state::open;
    }
}

std::shared_ptr<circuit_breaker> circuit_breaker_manager::create(const circuit_breaker::config& cfg)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
 
    const auto iter = circuitBreakers.find(cfg.name);
    if (iter == circuitBreakers.end())
    {
        std::shared_ptr<circuit_breaker> cb = std::make_shared<circuit_breaker>(cfg);
        circuitBreakers.emplace(cfg.name, std::move(cb));
    }

    return iter->second;
}

std::shared_ptr<circuit_breaker> circuit_breaker_manager::get(const std::string& name)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);

    const auto iter = circuitBreakers.find(name);
    if (iter == circuitBreakers.end())
    {
        circuit_breaker::config cfg;
        cfg.name = name;
        return create(cfg);
    }

    return iter->second;
}
} // shield