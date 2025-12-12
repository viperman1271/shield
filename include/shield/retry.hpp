#pragma once

#include <chrono>
#include <iostream>
#include <thread>

namespace shield
{
template<typename Func>
auto retry(Func&& func, int max_attempts = 3, std::chrono::milliseconds delay = std::chrono::milliseconds(100))
{
    using return_type = decltype(func());

    for (int attempt = 1; attempt <= max_attempts; ++attempt)
    {
        try
        {
            return func();
        }
        catch (const std::exception& e)
        {
            if (attempt == max_attempts)
            {
                throw;
            }
            std::cout << "Attempt " << attempt << " failed: " << e.what()
                << ". Retrying...\n";
            std::this_thread::sleep_for(delay);
            delay *= 2; // Exponential backoff
        }
    }

    throw std::runtime_error("Retry failed");
}

class retry_policy
{

};

static const retry_policy default_retry_policy;
} // shield