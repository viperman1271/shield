#include <shield/all.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <chrono>

TEST_CASE("Circuit breaker - initial state is closed", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 3, std::chrono::seconds(1));
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE("Circuit breaker - opens after threshold failures", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 3;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::seconds(10));
    
    // Fail 3 times to reach threshold
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        cb->on_failure();
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    REQUIRE(cb->get_failure_count() == MAX_FAILURE_COUNT);
}

TEST_CASE("Circuit breaker - rejects calls when open", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 2;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::seconds(10));
    
    // Open the circuit
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        cb->on_failure();
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    
    // Should reject immediately
    REQUIRE_FALSE(cb->on_execute_function());
}

TEST_CASE("Circuit breaker - transitions to half-open after timeout", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 2;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::milliseconds(100));
    
    // Open the circuit
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        cb->on_failure();
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Next call should transition to half-open
    cb->on_execute_function();
    cb->on_success();
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE("Circuit breaker - half-open closes on success", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 2;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::milliseconds(50));
    
    // Open the circuit
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        cb->on_failure();
    }
    
    // Wait and try again (transitions to half-open then closed)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    cb->on_execute_function();
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::half_open);
    cb->on_success(); // Succeed again in half-open state
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE("Circuit breaker - half-open reopens on failure", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 2;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::milliseconds(50));
    
    // Open the circuit
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        cb->on_failure();
    }
    
    // Wait for half-open transition
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    cb->on_execute_function();
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::half_open);
    cb->on_failure(); // Fail again in half-open state
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}

TEST_CASE("Circuit breaker - success resets failure count", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 5;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::seconds(1));
    
    // Fail a few times
    for (int i = 0; i < 3; i++)
    {
        cb->on_failure();
    }
    
    REQUIRE(cb->get_failure_count() == 3);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    
    // Succeed once
    cb->on_success();
    
    REQUIRE(cb->get_failure_count() == 0);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE("Circuit breaker - custom failure threshold", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 10;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::seconds(1));
    
    // Fail 9 times - should still be closed
    for (int i = 0; i < 9; i++)
    {
        cb->on_failure();
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    
    // 10th failure should open it
    cb->on_failure();
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}