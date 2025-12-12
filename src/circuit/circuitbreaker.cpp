#include <shield/circuitbreaker.hpp>

namespace shield
{
namespace detail
{
    class circuit_breaker final
    {
    public:
        circuit_breaker(const std::string& name, int failureThreshold, std::chrono::seconds timeout)
            : failureThreshold(failureThreshold)
            , timeout(timeout)
            , name(name)
            , failureCount(0)
            , state(shield::circuit_breaker::state::closed)
        {
        }

        circuit_breaker(const shield::circuit_breaker::config& cfg)
            : failureThreshold(cfg.failureThreshold)
            , name(cfg.name)
            , timeout(cfg.timeout)
            , failureCount(0)
            , state(shield::circuit_breaker::state::closed)
        {
        }

        shield::circuit_breaker::state get_state() const { return state; }
        int get_failure_count() const { return failureCount; }

        void on_success()
        {
            std::lock_guard<std::mutex> lock(mutex);
            failureCount = 0;
            if (state == shield::circuit_breaker::state::half_open)
            {
                std::cout << "[cb] Transitioning '" << name << "' from HALF_OPEN to CLOSED" << std::endl;
                state = shield::circuit_breaker::state::closed;
            }
        }

        void on_failure()
        {
            std::lock_guard<std::mutex> lock(mutex);
            ++failureCount;
            lastFailureTime = std::chrono::steady_clock::now();

            if (failureCount >= failureThreshold && state != shield::circuit_breaker::state::open)
            {
                std::cout << "[cb] Transitioning '" << name << "' OPEN" << std::endl;
                state = shield::circuit_breaker::state::open;
            }
        }

        void on_execute_function()
        {
            if (state == shield::circuit_breaker::state::open)
            {
                auto now = std::chrono::steady_clock::now();
                if (now - lastFailureTime > timeout)
                {
                    std::cout << "Circuit transitioning to HALF_OPEN\n";
                    state = shield::circuit_breaker::state::half_open;
                }
                else
                {
                    throw std::runtime_error("Circuit breaker is OPEN");
                }
            }
        }

    private:
        int failureThreshold;
        const std::string name;
        std::chrono::seconds timeout;
        std::atomic<int> failureCount;
        std::atomic<shield::circuit_breaker::state> state;
        std::chrono::steady_clock::time_point lastFailureTime;
        std::mutex mutex;
    };
} // detail

circuit_breaker::circuit_breaker(const std::string& name, int failureThreshold, std::chrono::seconds timeout)
    : pImpl(std::make_unique<shield::detail::circuit_breaker>(name, failureThreshold, timeout))
{
}

circuit_breaker::circuit_breaker(const config& cfg)
    : pImpl(std::make_unique<shield::detail::circuit_breaker>(cfg))
{
}

circuit_breaker::~circuit_breaker()
{
}

shield::circuit_breaker::state circuit_breaker::get_state() const
{
    return pImpl->get_state();
}

int circuit_breaker::get_failure_count() const
{
    return pImpl->get_failure_count();
}

void circuit_breaker::on_success()
{
    pImpl->on_success();
}

void circuit_breaker::on_failure()
{
    pImpl->on_failure();
}

void circuit_breaker::on_execute_function()
{
    pImpl->on_execute_function();
}

} // shield