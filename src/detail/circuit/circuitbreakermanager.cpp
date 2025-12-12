#include <shield/circuitbreaker.hpp>

#include <mutex>

namespace shield
{
namespace detail
{
class circuit_breaker_manager
{
public:
    std::shared_ptr<shield::circuit_breaker> create(const shield::circuit_breaker::config& cfg)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        const auto iter = circuitBreakers.find(cfg.name);
        if (iter == circuitBreakers.end())
        {
            std::shared_ptr<shield::circuit_breaker> cb = std::make_shared<shield::circuit_breaker>(cfg);
            circuitBreakers.emplace(cfg.name, std::move(cb));
        }

        return iter->second;
    }

    std::shared_ptr<shield::circuit_breaker> get(const std::string& name)
    {
        std::lock_guard<std::recursive_mutex> lock(mutex);

        const auto iter = circuitBreakers.find(name);
        if (iter == circuitBreakers.end())
        {
            shield::circuit_breaker::config cfg;
            cfg.name = name;
            return create(cfg);
        }

        return iter->second;
    }

private:
    std::recursive_mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<shield::circuit_breaker>> circuitBreakers;
};
} // detail

circuit_breaker_manager::circuit_breaker_manager()
    : pImpl(std::make_unique<detail::circuit_breaker_manager>())
{
}

std::shared_ptr<shield::circuit_breaker> circuit_breaker_manager::create(const circuit_breaker::config& cfg)
{
    return pImpl->create(cfg);
}

std::shared_ptr<shield::circuit_breaker> circuit_breaker_manager::get(const std::string& name)
{
    return pImpl->get(name);
}
}