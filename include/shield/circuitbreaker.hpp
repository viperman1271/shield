/* Copyright (c) 2025 Michael Filion
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files(the “Software”),
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

//#include <shield/all.hpp>

#include <shield/exceptions.hpp>
#include <shield/fallback.hpp>
#include <shield/retry.hpp>
#include <shield/timeout.hpp>

#include <chrono>
#include <iostream>
#include <mutex>
#include <unordered_map>

namespace shield
{
namespace detail
{
    class circuit_breaker;
}

class circuit_breaker final
{
public:
    struct config
    {
        config()
            : failureThreshold(5)
            , timeout(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(60)))
            , name("default")
        {
        }

        int failureThreshold;
        std::chrono::milliseconds timeout;
        std::string name;
    };

    enum class state
    {
        closed,
        open,
        half_open
    };

    template<typename Rep, typename Period>
    static std::shared_ptr<circuit_breaker> create(const std::string& name, int failureThreshold = 5, std::chrono::duration<Rep, Period> duration = std::chrono::seconds(60))
    {
        return create(name, failureThreshold, std::chrono::duration_cast<std::chrono::milliseconds>(duration));
    }

    static std::shared_ptr<shield::circuit_breaker> create(const std::string& name, int failureThreshold = 5, std::chrono::milliseconds timeout = std::chrono::seconds(60));
    static std::shared_ptr<shield::circuit_breaker> create(const config& cfg);
    
    ~circuit_breaker();

    void init(std::function<void(const std::string&, std::function<void()>, std::function<void()>, std::function<bool()>)> callback);

    state get_state() const;
    int get_failure_count() const;

    const std::string& get_name() const;

private:
    circuit_breaker(const std::string& name, int failureThreshold = 5, std::chrono::milliseconds timeout = std::chrono::seconds(60));
    circuit_breaker(const config& cfg);

private:
    void on_success();
    void on_failure();
    bool on_execute_function();

    std::unique_ptr<detail::circuit_breaker> pImpl;
};
} // shield