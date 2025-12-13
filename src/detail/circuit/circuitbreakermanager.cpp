#include <detail/circuit/circuitbreakermanager.hpp>

#include <mutex>

namespace
{
    static shield::detail::circuit_breaker_manager instance;
}

namespace shield
{
namespace detail
{
namespace impl
{
    class circuit_breaker_manager
    {
    public:
        struct circuit_registration final
        {
            std::shared_ptr<shield::circuit_breaker> instance;

            std::function<void()> successFunc;
            std::function<void()> failureFunc;
            std::function<bool()> executeFunc;
        };

    public:
        std::shared_ptr<shield::circuit_breaker> create(const shield::circuit_breaker::config& cfg)
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            const auto iter = circuitBreakers.find(cfg.name);
            if (iter == circuitBreakers.end())
            {
                circuit_registration registration
                {
                    .instance = shield::circuit_breaker::create(cfg)
                };

                auto [addedIter, added] = circuitBreakers.emplace(cfg.name, std::move(registration));
                addedIter->second.instance->init(std::bind(&circuit_breaker_manager::register_instance, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

                return addedIter->second.instance;
            }

            return iter->second.instance;
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

            return iter->second.instance;
        }

        void clear()
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            circuitBreakers.clear();
        }

        void on_success(std::shared_ptr<shield::circuit_breaker> cb) const
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            const auto iter = circuitBreakers.find(cb->get_name());
            if (iter != circuitBreakers.end())
            {
                iter->second.successFunc();
            }
        }

        void on_failure(std::shared_ptr<shield::circuit_breaker> cb) const
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            const auto iter = circuitBreakers.find(cb->get_name());
            if (iter != circuitBreakers.end())
            {
                iter->second.failureFunc();
            }
        }

        bool on_execute_function(std::shared_ptr<shield::circuit_breaker> cb) const
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            const auto iter = circuitBreakers.find(cb->get_name());
            if (iter != circuitBreakers.end())
            {
                return iter->second.executeFunc();
            }

            return false;
        }

        void register_circuit_breaker(std::shared_ptr<shield::circuit_breaker> cb)
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            const auto iter = circuitBreakers.find(cb->get_name());
            if (iter == circuitBreakers.end())
            {
                circuit_registration registration
                {
                    .instance = cb
                };

                auto [addedIter, added] = circuitBreakers.emplace(cb->get_name(), std::move(registration));
                addedIter->second.instance->init(std::bind(&circuit_breaker_manager::register_instance, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            }
        }

    private:
        void register_instance(const std::string& name, std::function<void()> successFunc, std::function<void()> failureFunc, std::function<bool()> executeFunc)
        {
            std::lock_guard<std::recursive_mutex> lock(mutex);

            const auto iter = circuitBreakers.find(name);
            if (iter != circuitBreakers.end())
            {
                iter->second.successFunc = successFunc;
                iter->second.failureFunc = failureFunc;
                iter->second.executeFunc = executeFunc;
            }
        }

    private:
        mutable std::recursive_mutex mutex;
        std::unordered_map<std::string, circuit_registration> circuitBreakers;
    };
} // impl

circuit_breaker_manager::circuit_breaker_manager()
    : pImpl(std::make_unique<impl::circuit_breaker_manager>())
{
}

std::shared_ptr<shield::circuit_breaker> circuit_breaker_manager::create(const shield::circuit_breaker::config& cfg)
{
    return pImpl->create(cfg);
}

std::shared_ptr<shield::circuit_breaker> circuit_breaker_manager::get(const std::string& name)
{
    return pImpl->get(name);
}

circuit_breaker_manager& circuit_breaker_manager::get_instance()
{
    return instance;
}

void circuit_breaker_manager::clear()
{
    return pImpl->clear();
}

void circuit_breaker_manager::on_success(std::shared_ptr<shield::circuit_breaker> cb) const
{
    pImpl->on_success(cb);
}

void circuit_breaker_manager::on_failure(std::shared_ptr<shield::circuit_breaker> cb) const
{
    pImpl->on_failure(cb);
}

bool circuit_breaker_manager::on_execute_function(std::shared_ptr<shield::circuit_breaker> cb) const
{
    return pImpl->on_execute_function(cb);
}

void circuit_breaker_manager::register_circuit_breaker(std::shared_ptr<shield::circuit_breaker> cb)
{
    pImpl->register_circuit_breaker(cb);
}

} // detail
} // shield