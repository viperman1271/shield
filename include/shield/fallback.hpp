/* Copyright (c) 2025 Michael Filion
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files(the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and /or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <shield/exceptions.hpp>

#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace shield
{
enum class fallback_type 
{
    DEFAULT,        ///< Returns the default value for the type
    SPECIFIC_VALUE, ///< Returns a pre-configured specific value
    CALLABLE,       ///< Invokes a fallback function to compute and return a value
    THROW,          ///< Will throw an exception rather than trying to provide a value
};

struct fallback_policy
{
public:
    using callable_type = std::function<std::any()>;

public:
    static fallback_policy with_default();

    template<typename T>
    static fallback_policy with_value(T&& value) 
    {
        return fallback_policy(fallback_type::SPECIFIC_VALUE, std::make_any<std::decay_t<T>>(std::forward<T>(value)));
    }

    static fallback_policy with_callable(callable_type fallback_function);

    template<typename Callable>
    static fallback_policy with_typed_callable(Callable&& fallback_fn) 
    {
        return with_callable([fn = std::forward<Callable>(fallback_fn)]() -> std::any 
        {
            return fn();
        });
    }

    static fallback_policy with_throw();

    template<typename T>
    requires (!std::is_void_v<T>)
    std::optional<T> get_value() const 
    {
        switch (fallbackType)
        {
        case fallback_type::THROW:
            throw shield::fallback_exception();
        }

        try 
        {
            switch (fallbackType) 
            {
            case fallback_type::DEFAULT:
                // For DEFAULT, return a default-constructed value
                if constexpr (std::is_default_constructible_v<T>) 
                {
                    return T{};
                }
                else 
                {
                    return std::nullopt;
                }

            case fallback_type::SPECIFIC_VALUE:
                // Try to extract the value from std::any
                if (specificValue.type() == typeid(T)) 
                {
                    return std::any_cast<T>(specificValue);
                }
                return std::nullopt;

            case fallback_type::CALLABLE:
                // Invoke the callable and try to extract the result
                if (fallbackCallable) 
                {
                    std::any result = fallbackCallable();
                    if (result.type() == typeid(T)) 
                    {
                        return std::any_cast<T>(result);
                    }
                }
                return std::nullopt;

            default:
                return std::nullopt;
            }
        }
        catch (...) 
        {
            // Catch all exceptions and return nullopt (noexcept guarantee)
            return std::nullopt;
        }
    }

    template<typename T>
    requires (std::is_void_v<T>)
    void get_value() const 
    {
        switch (fallbackType) 
        {
        case fallback_type::DEFAULT:
            return;

        case fallback_type::SPECIFIC_VALUE:
            return;

        case fallback_type::CALLABLE:
            // Invoke the callable and try to extract the result
            if (fallbackCallable) 
            {
                fallbackCallable();
            }
            return;

        case fallback_type::THROW:
            throw shield::fallback_exception();

        default:
            return;
        }
    }

    template<typename T>
    T get_value_or(T&& default_value) const noexcept 
    {
        std::optional<T> result = get_value<std::decay_t<T>>();
        if (result) 
        {
            return std::move(*result);
        }
        return std::forward<T>(default_value);
    }

    fallback_type get_type() const noexcept { return fallbackType; }
    bool has_specific_value() const noexcept { return fallbackType == fallback_type::SPECIFIC_VALUE && specificValue.has_value(); }
    bool has_callable() const noexcept { return fallbackType == fallback_type::CALLABLE && static_cast<bool>(fallbackCallable); }

    const std::type_info& stored_type() const noexcept;

    template<typename T>
    bool can_cast_to() const noexcept 
    {
        if (fallbackType == fallback_type::DEFAULT) 
        {
            return std::is_default_constructible_v<T>;
        }
        if (fallbackType == fallback_type::SPECIFIC_VALUE) 
        {
            return specificValue.type() == typeid(T);
        }
        // For CALLABLE, we can't know without executing
        return true;
    }

private:
    fallback_policy(fallback_type type, std::any specific_value = std::any{}, callable_type fallback_callable = nullptr)
        : fallbackType(type)
        , specificValue(std::move(specific_value))
        , fallbackCallable(std::move(fallback_callable))
    {
        validate();
    }

    void validate() const;

private:
    fallback_type fallbackType;
    std::any specificValue;
    callable_type fallbackCallable;
};

/**
 * @brief Converts fallback_type enum to string representation.
 *
 * @param type The fallback_type to convert
 * @return String representation of the fallback type
 */
std::string to_string(fallback_type type);

static const fallback_policy default_fallback_policy = fallback_policy::with_default();
}