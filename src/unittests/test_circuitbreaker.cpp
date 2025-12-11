#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include "shield_cpp.hpp"

using namespace shield_cpp;

TEST_CASE("Circuit breaker - initial state is closed", "[circuit_breaker]")
{
    circuit_breaker cb(3, std::chrono::seconds(1));
    
    REQUIRE(cb.get_state() == circuit_breaker::state::closed);
    REQUIRE(cb.get_failure_count() == 0);
}

TEST_CASE("Circuit breaker - successful execution", "[circuit_breaker]")
{
    circuit_breaker cb(3, std::chrono::seconds(1));
    
    auto result = cb.execute([]()
    {
        return 42;
    });
    
    REQUIRE(result == 42);
    REQUIRE(cb.get_state() == circuit_breaker::state::closed);
    REQUIRE(cb.get_failure_count() == 0);
}

TEST_CASE("Circuit breaker - opens after threshold failures", "[circuit_breaker]")
{
    circuit_breaker cb(3, std::chrono::seconds(10));
    
    // Fail 3 times to reach threshold
    for (int i = 0; i < 3; i++)
    {
        try
        {
            cb.execute([]()
            {
                throw std::runtime_error("Service failure");
                return 0;
            });
        }
        catch (const std::runtime_error&)
        {
            // Expected
        }
    }
    
    REQUIRE(cb.get_state() == circuit_breaker::state::open);
    REQUIRE(cb.get_failure_count() == 3);
}

TEST_CASE("Circuit breaker - rejects calls when open", "[circuit_breaker]")
{
    circuit_breaker cb(2, std::chrono::seconds(10));
    
    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            cb.execute([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    REQUIRE(cb.get_state() == circuit_breaker::state::open);
    
    // Should reject immediately
    REQUIRE_THROWS_WITH(
        cb.execute([]() { return 42; }),
        "Circuit breaker is OPEN"
    );
}

TEST_CASE("Circuit breaker - transitions to half-open after timeout", "[circuit_breaker]")
{
    circuit_breaker cb(2, std::chrono::milliseconds(100));
    
    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            cb.execute([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    REQUIRE(cb.get_state() == circuit_breaker::state::open);
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Next call should transition to half-open
    auto result = cb.execute([]() { return 99; });
    
    REQUIRE(result == 99);
    REQUIRE(cb.get_state() == circuit_breaker::state::closed);
    REQUIRE(cb.get_failure_count() == 0);
}

TEST_CASE("Circuit breaker - half-open closes on success", "[circuit_breaker]")
{
    circuit_breaker cb(2, std::chrono::milliseconds(50));
    
    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            cb.execute([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    // Wait and try again (transitions to half-open then closed)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto result = cb.execute([]() { return 123; });
    
    REQUIRE(result == 123);
    REQUIRE(cb.get_state() == circuit_breaker::state::closed);
}

TEST_CASE("Circuit breaker - half-open reopens on failure", "[circuit_breaker]")
{
    circuit_breaker cb(2, std::chrono::milliseconds(50));
    
    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            cb.execute([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    // Wait for half-open transition
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Fail again in half-open state
    try
    {
        cb.execute([]()
        {
            throw std::runtime_error("Still failing");
            return 0;
        });
    }
    catch (...) {}
    
    REQUIRE(cb.get_state() == circuit_breaker::state::open);
}

TEST_CASE("Circuit breaker - success resets failure count", "[circuit_breaker]")
{
    circuit_breaker cb(5, std::chrono::seconds(1));
    
    // Fail a few times
    for (int i = 0; i < 3; i++)
    {
        try
        {
            cb.execute([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    REQUIRE(cb.get_failure_count() == 3);
    REQUIRE(cb.get_state() == circuit_breaker::state::closed);
    
    // Succeed once
    cb.execute([]() { return 42; });
    
    REQUIRE(cb.get_failure_count() == 0);
    REQUIRE(cb.get_state() == circuit_breaker::state::closed);
}

TEST_CASE("Circuit breaker - custom failure threshold", "[circuit_breaker]")
{
    circuit_breaker cb(10, std::chrono::seconds(1));
    
    // Fail 9 times - should still be closed
    for (int i = 0; i < 9; i++)
    {
        try
        {
            cb.execute([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    REQUIRE(cb.get_state() == circuit_breaker::state::closed);
    
    // 10th failure should open it
    try
    {
        cb.execute([]()
        {
            throw std::runtime_error("Fail");
            return 0;
        });
    }
    catch (...) {}
    
    REQUIRE(cb.get_state() == circuit_breaker::state::open);
}