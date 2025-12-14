#include <shield/all.hpp>

#include <detail/circuit/circuitbreakermanager.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <chrono>

struct circuit_breaker_test_fixture
{
public:
    ~circuit_breaker_test_fixture()
    {
        shield::detail::circuit_breaker_manager::get_instance().clear();
    }

protected:
    bool on_execute_function(std::shared_ptr<shield::circuit_breaker> cb)
    {
        return shield::detail::circuit_breaker_manager::get_instance().on_execute_function(cb);
    }

    void on_failure(std::shared_ptr<shield::circuit_breaker> cb)
    {
        shield::detail::circuit_breaker_manager::get_instance().on_failure(cb);
    }

    void on_success(std::shared_ptr<shield::circuit_breaker> cb)
    {
        shield::detail::circuit_breaker_manager::get_instance().on_success(cb);
    }
};

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - initialization with default config", "[circuit_breaker][init]")
{
    shield::circuit_breaker::config cfg;
    cfg.name = "init-test-default";

    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create(cfg);

    // Verify default configuration values
    REQUIRE(cb->get_name() == "init-test-default");
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);

    // Verify circuit accepts execution when freshly initialized
    REQUIRE(on_execute_function(cb) == true);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - initialization with custom parameters", "[circuit_breaker][init]")
{
    const int custom_threshold = 7;
    const std::chrono::milliseconds custom_timeout = std::chrono::seconds(30);
    const std::string custom_name = "init-test-custom";

    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create(custom_name, custom_threshold, custom_timeout);

    REQUIRE(cb->get_name() == custom_name);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);

    // Verify circuit accepts execution when freshly initialized
    REQUIRE(on_execute_function(cb) == true);

    // Verify threshold is respected
    for (int i = 0; i < custom_threshold - 1; ++i)
    {
        on_failure(cb);
    }
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == custom_threshold - 1);

    // One more failure should open it
    on_failure(cb);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    REQUIRE(cb->get_failure_count() == custom_threshold);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - on_execute_function returns true when closed", "[circuit_breaker][init]")
{
    const std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("exec-test", 5, std::chrono::seconds(10));

    // Should return true when circuit is closed
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(on_execute_function(cb) == true);

    // Multiple calls should all return true
    for (int i = 0; i < 10; ++i)
    {
        REQUIRE(on_execute_function(cb) == true);
    }
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - on_execute_function returns false when open", "[circuit_breaker][init]")
{
    const int threshold = 3;
    const std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("exec-open-test", threshold, std::chrono::seconds(10));

    // Open the circuit
    for (int i = 0; i < threshold; ++i)
    {
        on_failure(cb);
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    REQUIRE(on_execute_function(cb) == false);

    // Multiple calls should all return false
    for (int i = 0; i < 10; ++i)
    {
        REQUIRE(on_execute_function(cb) == false);
    }
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - state transitions work correctly after initialization", "[circuit_breaker][init]")
{
    const int threshold = 2;
    const std::chrono::milliseconds timeout = std::chrono::milliseconds(100);

    const std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("transition-test", threshold, timeout);

    // Initial state
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);

    // Transition to open
    for (int i = 0; i < threshold; ++i)
    {
        on_failure(cb);
    }
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Should transition to half-open on next execute check
    on_execute_function(cb);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::half_open);

    // Success should close it
    on_success(cb);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - multiple instances with same name return same instance", "[circuit_breaker][init]")
{
    const std::shared_ptr<shield::circuit_breaker> cb1 = shield::circuit_breaker::create("shared-instance", 5, std::chrono::seconds(10));
    const std::shared_ptr<shield::circuit_breaker> cb2 = shield::circuit_breaker::create("shared-instance", 5, std::chrono::seconds(10));

    // Should be the exact same instance
    REQUIRE(cb1.get() == cb2.get());

    // Changes to one should affect the other
    on_failure(cb1);
    REQUIRE(cb2->get_failure_count() == 1);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - different names create different instances", "[circuit_breaker][init]")
{
    std::shared_ptr<shield::circuit_breaker> cb1 =
        shield::circuit_breaker::create("instance-1", 5, std::chrono::seconds(10));

    std::shared_ptr<shield::circuit_breaker> cb2 =
        shield::circuit_breaker::create("instance-2", 5, std::chrono::seconds(10));

    // Should be different instances
    REQUIRE(cb1.get() != cb2.get());

    // Changes to one should NOT affect the other
    on_failure(cb1);
    REQUIRE(cb1->get_failure_count() == 1);
    REQUIRE(cb2->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - on_success resets failure count", "[circuit_breaker][init]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("reset-test", 10, std::chrono::seconds(10));

    // Accumulate some failures
    for (int i = 0; i < 5; ++i)
    {
        on_failure(cb);
    }
    REQUIRE(cb->get_failure_count() == 5);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);

    // Success should reset
    on_success(cb);
    REQUIRE(cb->get_failure_count() == 0);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - freshly created circuit can handle immediate failure", "[circuit_breaker][init]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("immediate-fail", 3, std::chrono::seconds(10));

    // Immediately fail without any prior success
    REQUIRE(on_execute_function(cb) == true);
    on_failure(cb);

    REQUIRE(cb->get_failure_count() == 1);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);

    // Should still allow execution
    REQUIRE(on_execute_function(cb) == true);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - rapid successive on_execute_function calls", "[circuit_breaker][init]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("rapid-exec", 5, std::chrono::milliseconds(100));

    // Rapid successive calls when closed should all return true
    for (int i = 0; i < 100; ++i)
    {
        REQUIRE(on_execute_function(cb) == true);
    }

    // Open the circuit
    for (int i = 0; i < 5; ++i)
    {
        on_failure(cb);
    }

    // Rapid successive calls when open should all return false
    for (int i = 0; i < 100; ++i)
    {
        REQUIRE(on_execute_function(cb) == false);
    }
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - timeout parameter is respected", "[circuit_breaker][init]")
{
    const std::chrono::milliseconds short_timeout = std::chrono::milliseconds(50);
    const int threshold = 2;

    std::shared_ptr<shield::circuit_breaker> cb =
        shield::circuit_breaker::create("timeout-test", threshold, short_timeout);

    // Open the circuit
    for (int i = 0; i < threshold; ++i)
    {
        on_failure(cb);
    }
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Before timeout - should stay open
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    REQUIRE(on_execute_function(cb) == false);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // After timeout - should transition to half-open
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    on_execute_function(cb);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::half_open);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - zero failures initially", "[circuit_breaker][init]")
{
    for (int i = 0; i < 10; ++i)
    {
        std::string name = "zero-fail-" + std::to_string(i);
        std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create(name, 5, std::chrono::seconds(10));

        REQUIRE(cb->get_failure_count() == 0);
        REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
        REQUIRE(on_execute_function(cb) == true);
    }
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - config struct initialization", "[circuit_breaker][init]")
{
    shield::circuit_breaker::config cfg;

    // Verify default config values
    REQUIRE(cfg.failureThreshold == 5);
    REQUIRE(cfg.timeout == std::chrono::milliseconds(std::chrono::seconds(60)));
    REQUIRE(cfg.name == "default");

    // Create with default config
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create(cfg);

    REQUIRE(cb->get_name() == "default");
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - deterministic behavior after creation", "[circuit_breaker][init]")
{
    // Run the same test multiple times to ensure deterministic behavior
    for (int iteration = 0; iteration < 20; ++iteration)
    {
        shield::detail::circuit_breaker_manager::get_instance().clear();

        std::string name = "deterministic-" + std::to_string(iteration);
        std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create(name, 3, std::chrono::seconds(10));

        // Every iteration should have identical initial state
        REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
        REQUIRE(cb->get_failure_count() == 0);
        REQUIRE(on_execute_function(cb) == true);

        // Fail it once
        on_failure(cb);
        REQUIRE(cb->get_failure_count() == 1);
        REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
        REQUIRE(on_execute_function(cb) == true);
    }
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - initial state is closed", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 3, std::chrono::seconds(1));
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - opens after threshold failures", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 3;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::seconds(10));
    
    // Fail 3 times to reach threshold
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        on_failure(cb);
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    REQUIRE(cb->get_failure_count() == MAX_FAILURE_COUNT);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - rejects calls when open", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 2;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::seconds(10));
    
    // Open the circuit
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        on_failure(cb);
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    
    // Should reject immediately
    REQUIRE_FALSE(on_execute_function(cb));
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - transitions to half-open after timeout", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 2;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::milliseconds(100));
    
    // Open the circuit
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        on_failure(cb);
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Next call should transition to half-open
    on_execute_function(cb);
    on_success(cb);
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - half-open closes on success", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 2;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::milliseconds(50));
    
    // Open the circuit
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        on_failure(cb);
    }
    
    // Wait and try again (transitions to half-open then closed)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    on_execute_function(cb);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::half_open);
    on_success(cb); // Succeed again in half-open state
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - half-open reopens on failure", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 2;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::milliseconds(50));
    
    // Open the circuit
    for (int i = 0; i < MAX_FAILURE_COUNT; i++)
    {
        on_failure(cb);
    }
    
    // Wait for half-open transition
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    on_execute_function(cb);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::half_open);
    on_failure(cb); // Fail again in half-open state
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - success resets failure count", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 5;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::seconds(1));
    
    // Fail a few times
    for (int i = 0; i < 3; i++)
    {
        on_failure(cb);
    }
    
    REQUIRE(cb->get_failure_count() == 3);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    
    // Succeed once
    on_success(cb);
    
    REQUIRE(cb->get_failure_count() == 0);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE_METHOD(circuit_breaker_test_fixture, "Circuit breaker - custom failure threshold", "[circuit_breaker]")
{
    const int MAX_FAILURE_COUNT = 10;
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", MAX_FAILURE_COUNT, std::chrono::seconds(1));
    
    // Fail 9 times - should still be closed
    for (int i = 0; i < 9; i++)
    {
        on_failure(cb);
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    
    // 10th failure should open it
    on_failure(cb);
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}