#pragma once

#include <shield/circuitbreaker.hpp>

namespace shield
{
namespace detail
{
namespace impl
{
    class circuit_breaker_manager;
} // impl

class circuit_breaker_manager final
{
public:
    circuit_breaker_manager();
    ~circuit_breaker_manager() = default;

    std::shared_ptr<shield::circuit_breaker> create(const shield::circuit_breaker::config& cfg);
    std::shared_ptr<shield::circuit_breaker> create(const std::string& name);
    std::shared_ptr<shield::circuit_breaker> get(const std::string& name);
    std::shared_ptr<shield::circuit_breaker> get_or_create(const std::string& name);

    static circuit_breaker_manager& get_instance();

    void clear();

    void on_success(std::shared_ptr<shield::circuit_breaker> cb) const;
    void on_failure(std::shared_ptr<shield::circuit_breaker> cb) const;
    bool on_execute_function(std::shared_ptr<shield::circuit_breaker> cb) const;

    void register_circuit_breaker(std::shared_ptr<shield::circuit_breaker> circuitBreaker);

private:
    std::unique_ptr<detail::impl::circuit_breaker_manager> pImpl;
};
} // detail
} // shield