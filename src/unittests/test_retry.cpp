#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include "shield_cpp.hpp"

using namespace shield_cpp;

TEST_CASE("Retry pattern - successful on first attempt", "[retry]")
{
    int call_count = 0;
    
    auto result = retry([&call_count]()
    {
        call_count++;
        return 42;
    }, 3);
    
    REQUIRE(result == 42);
    REQUIRE(call_count == 1);
}

TEST_CASE("Retry pattern - successful after retries", "[retry]")
{
    int call_count = 0;
    
    auto result = retry([&call_count]()
    {
        call_count++;
        if (call_count < 3)
        {
            throw std::runtime_error("Temporary failure");
        }
        return 100;
    }, 5);
    
    REQUIRE(result == 100);
    REQUIRE(call_count == 3);
}

TEST_CASE("Retry pattern - exhausts all attempts", "[retry]")
{
    int call_count = 0;
    
    REQUIRE_THROWS_AS(
        retry([&call_count]()
        {
            call_count++;
            throw std::runtime_error("Permanent failure");
            return 0;
        }, 3),
        std::runtime_error
    );
    
    REQUIRE(call_count == 3);
}

TEST_CASE("Retry pattern - exponential backoff timing", "[retry]")
{
    int call_count = 0;
    auto start = std::chrono::steady_clock::now();
    
    try
    {
        retry([&call_count]()
        {
            call_count++;
            throw std::runtime_error("Always fail");
            return 0;
        }, 3, std::chrono::milliseconds(10));
    }
    catch (...)
    {
        // Expected to fail
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should have delays: 10ms + 20ms = 30ms minimum (2 retries after first attempt)
    REQUIRE(duration.count() >= 30);
    REQUIRE(call_count == 3);
}

TEST_CASE("Retry pattern - void return type", "[retry]")
{
    int call_count = 0;
    
    REQUIRE_NOTHROW(
        retry([&call_count]()
        {
            call_count++;
            if (call_count < 2)
            {
                throw std::runtime_error("Fail once");
            }
        }, 3)
    );
    
    REQUIRE(call_count == 2);
}

TEST_CASE("Retry pattern - single attempt", "[retry]")
{
    int call_count = 0;
    
    REQUIRE_THROWS_AS(
        retry([&call_count]()
        {
            call_count++;
            throw std::runtime_error("Fail");
            return 0;
        }, 1),
        std::runtime_error
    );
    
    REQUIRE(call_count == 1);
}

TEST_CASE("Retry pattern - returns correct value type", "[retry]")
{
    auto string_result = retry([]()
    {
        return std::string("success");
    }, 3);
    
    REQUIRE(string_result == "success");
    
    auto double_result = retry([]()
    {
        return 3.14159;
    }, 3);
    
    REQUIRE(double_result == 3.14159);
}