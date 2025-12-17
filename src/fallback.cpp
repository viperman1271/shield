#include <shield/fallback.hpp>

namespace shield
{
fallback_policy fallback_policy::with_default()
{
    return fallback_policy(fallback_type::DEFAULT);
}

fallback_policy fallback_policy::with_callable(callable_type fallback_function)
{
    if (!fallback_function)
    {
        throw std::invalid_argument("fallback_function cannot be null");
    }
    return fallback_policy(fallback_type::CALLABLE, std::any{}, std::move(fallback_function));
}

fallback_policy fallback_policy::with_throw()
{
    return fallback_policy(fallback_type::THROW);
}

const std::type_info& fallback_policy::stored_type() const noexcept
{
    if (fallbackType == fallback_type::SPECIFIC_VALUE && specificValue.has_value())
    {
        return specificValue.type();
    }
    return typeid(void);
}

void fallback_policy::validate() const
{
    if (fallbackType == fallback_type::SPECIFIC_VALUE && !specificValue.has_value())
    {
        throw std::invalid_argument("specific_value must be provided for SPECIFIC_VALUE fallback type");
    }

    if (fallbackType == fallback_type::CALLABLE && !fallbackCallable)
    {
        throw std::invalid_argument("fallback_callable must be provided for CALLABLE fallback type");
    }
}

std::string to_string(fallback_type type)
{
    switch (type)
    {
    case fallback_type::DEFAULT:
        return "DEFAULT";
    case fallback_type::SPECIFIC_VALUE:
        return "SPECIFIC_VALUE";
    case fallback_type::CALLABLE:
        return "CALLABLE";
    case fallback_type::THROW:
        return "THROW";
    default:
        return "UNKNOWN";
    }
}

} // shield