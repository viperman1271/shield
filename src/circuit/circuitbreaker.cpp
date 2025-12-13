#include <shield/circuitbreaker.hpp>

#include <detail/circuit/circuitbreakermanager.hpp>

namespace shield
{
namespace detail
{
    class circuit_breaker final
    {
    public:
        circuit_breaker(const std::string& name, int failureThreshold, std::chrono::milliseconds timeout)
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
        const std::string& get_name() const { return name; }

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

        bool on_execute_function()
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
                    //throw std::runtime_error("Circuit breaker is OPEN");
                    return false;
                }
            }

            return state != shield::circuit_breaker::state::open;
        }

        void init(std::function<void(const std::string&, std::function<void()>, std::function<void()>, std::function<void()>)> callback)
        {
            callback(name, std::bind(&circuit_breaker::on_success, this), std::bind(&circuit_breaker::on_failure, this), std::bind(&circuit_breaker::on_execute_function, this));
        }

    private:
        int failureThreshold;
        const std::string name;
        std::chrono::milliseconds timeout;
        std::atomic<int> failureCount;
        std::atomic<shield::circuit_breaker::state> state;
        std::chrono::steady_clock::time_point lastFailureTime;
        std::mutex mutex;
    };
} // detail

std::shared_ptr<shield::circuit_breaker> circuit_breaker::create(const std::string& name, int failureThreshold, std::chrono::milliseconds timeout)
{
    std::shared_ptr<shield::circuit_breaker> newInstance(new shield::circuit_breaker(name, failureThreshold, timeout));
    detail::circuit_breaker_manager::get_instance().register_circuit_breaker(newInstance);
    return newInstance;
}

std::shared_ptr<shield::circuit_breaker> circuit_breaker::create(const config& cfg)
{
    std::shared_ptr<shield::circuit_breaker> newInstance(new shield::circuit_breaker(cfg));
    detail::circuit_breaker_manager::get_instance().register_circuit_breaker(newInstance);
    return newInstance;
}

circuit_breaker::circuit_breaker(const std::string& name, int failureThreshold, std::chrono::milliseconds timeout)
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

void circuit_breaker::init(std::function<void(const std::string&, std::function<void()>, std::function<void()>, std::function<void()>)> callback)
{
    pImpl->init(callback);
}

shield::circuit_breaker::state circuit_breaker::get_state() const
{
    return pImpl->get_state();
}

int circuit_breaker::get_failure_count() const
{
    return pImpl->get_failure_count();
}

const std::string& circuit_breaker::get_name() const
{
    return pImpl->get_name();
}

void circuit_breaker::on_success()
{
    pImpl->on_success();
}

void circuit_breaker::on_failure()
{
    pImpl->on_failure();
}

bool circuit_breaker::on_execute_function()
{
    return pImpl->on_execute_function();
}

} // shield