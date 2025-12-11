#include "shield_cpp.hpp"
#include <iostream>
#include <string>

using namespace shield_cpp;

// Simulate an unreliable service
int unreliable_service_call(int& call_count)
{
    call_count++;
    if (call_count < 3)
    {
        throw std::runtime_error("Service temporarily unavailable");
    }
    return 42;
}

// Simulate a slow service
std::string slow_service_call()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return "Success from slow service";
}

int main()
{
    std::cout << "=== Shield C++ - Resilience Patterns Demo ===\n\n";
    
    // 1. RETRY PATTERN
    std::cout << "1. RETRY PATTERN:\n";
    std::cout << "   Attempting unreliable service call...\n";
    try
    {
        int attempt = 0;
        auto result = retry([&attempt]()
        {
            return unreliable_service_call(attempt);
        }, 5, std::chrono::milliseconds(50));
        
        std::cout << "   ✓ Success after retries! Result: " << result << "\n\n";
    }
    catch (const std::exception& e)
    {
        std::cout << "   ✗ Failed: " << e.what() << "\n\n";
    }
    
    // 2. CIRCUIT BREAKER PATTERN
    std::cout << "2. CIRCUIT BREAKER PATTERN:\n";
    circuit_breaker cb(3, std::chrono::seconds(2));
    
    std::cout << "   Simulating service failures...\n";
    for (int i = 0; i < 5; i++)
    {
        try
        {
            cb.execute([]()
            {
                throw std::runtime_error("Service unavailable");
                return 0;
            });
        }
        catch (const std::exception& e)
        {
            std::cout << "   Call " << (i+1) << ": " << e.what() << "\n";
        }
    }
    std::cout << "   Circuit state: " 
              << (cb.get_state() == circuit_breaker::state::open ? "OPEN" : "CLOSED")
              << "\n\n";
    
    // 3. TIMEOUT PATTERN
    std::cout << "3. TIMEOUT PATTERN:\n";
    std::cout << "   Calling slow service with timeout...\n";
    try
    {
        auto result = with_timeout([]()
        {
            return slow_service_call();
        }, std::chrono::seconds(1));
        
        std::cout << "   ✓ " << result << "\n\n";
    }
    catch (const std::exception& e)
    {
        std::cout << "   ✗ " << e.what() << "\n\n";
    }
    
    // 4. FALLBACK PATTERN
    std::cout << "4. FALLBACK PATTERN:\n";
    std::cout << "   Attempting primary service with fallback...\n";
    auto result = with_fallback(
        []() -> std::string
        {
            throw std::runtime_error("Primary service failed");
        },
        []()
        {
            return std::string("Using cached data");
        }
    );
    std::cout << "   Result: " << result << "\n\n";
    
    // 5. COMBINED PATTERN
    std::cout << "5. COMBINED PATTERN (Retry + Timeout + Fallback):\n";
    std::cout << "   Building resilient service call...\n";
    
    int combined_attempts = 0;
    auto final_result = with_fallback(
        [&combined_attempts]()
        {
            return retry([&combined_attempts]()
            {
                combined_attempts++;
                return with_timeout([]()
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    return std::string("Primary service response");
                }, std::chrono::milliseconds(100));
            }, 3);
        },
        []()
        {
            return std::string("Fallback response from cache");
        }
    );
    
    std::cout << "   Result: " << final_result << "\n";
    std::cout << "   Attempts made: " << combined_attempts << "\n\n";
    
    // 6. RESILIENT SERVICE WITH METRICS
    std::cout << "6. RESILIENT SERVICE WITH PROMETHEUS METRICS:\n";
    auto registry = std::make_shared<prometheus::Registry>();
    resilient_service service(registry);
    
    std::cout << "   Executing resilient API calls...\n";
    for (int i = 0; i < 3; i++)
    {
        auto api_result = service.execute_resilient(
            [i]()
            {
                if (i == 1)
                {
                    throw std::runtime_error("Simulated failure");
                }
                return std::string("API response #") + std::to_string(i);
            },
            []()
            {
                return std::string("Fallback data");
            }
        );
        
        std::cout << "   Call " << (i+1) << ": " << api_result << "\n";
    }
    
    std::cout << "\n=== Demo Complete ===\n";
    std::cout << "Check out the unit tests for comprehensive examples!\n";
    
    return 0;
}