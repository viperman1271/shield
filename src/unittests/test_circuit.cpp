#include <shield/all.hpp>
#include <detail/circuit/circuitbreakermanager.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <chrono>

struct circuit_test_fixture
{
public:
    ~circuit_test_fixture()
    {
        shield::detail::circuit_breaker_manager::get_instance().clear();
    }
};

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - initial state is closed", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 3, std::chrono::seconds(1));

    shield::circuit("test").run([]
    {
        std::cout << "std::cout" << std::endl;
        return true;
    });
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - successful execution", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 3, std::chrono::seconds(1));
    
    int result = shield::circuit("test").run([]()
    {
        return 42;
    });
    
    REQUIRE(result == 42);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - opens after threshold failures", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 3, std::chrono::seconds(10));
    
    // Fail 3 times to reach threshold
    for (int i = 0; i < 3; i++)
    {
        try
        {
            shield::circuit("test").run([]()
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
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    REQUIRE(cb->get_failure_count() == 3);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - rejects calls when open", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::seconds(10));
    
    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("test").run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    
    // Should reject immediately
    REQUIRE_THROWS_WITH(
        shield::circuit("test").run([]() { return 42; }),
        "Circuit is OPEN"
    );
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - transitions to half-open after timeout", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::milliseconds(100));
    
    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("test").run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Next call should transition to half-open
    auto result = shield::circuit("test").run([]() { return 99; });
    
    REQUIRE(result == 99);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - transitions to half-open after timeout with explicit exception", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::milliseconds(100));

    //try
    {
        // Open the circuit
        for (int i = 0; i < 2; i++)
        {
            shield::circuit("test").run<std::runtime_error>([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
    }
    //catch (const std::exception& ex) 
    {
        //std::cout << ex.what() << std::endl;

    }

    

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Next call should transition to half-open
    auto result = shield::circuit("test").run([]() { return 99; });

    REQUIRE(result == 99);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - half-open closes on success", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::milliseconds(50));
    
    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("test").run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) 
        {
        }
    }
    
    // Wait and try again (transitions to half-open then closed)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto result = shield::circuit("test").run([]() { return 123; });
    
    REQUIRE(result == 123);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - half-open closes on success with explicit exception", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::milliseconds(50));

    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        shield::circuit("test").run<std::runtime_error>([]()
        {
            throw std::runtime_error("Fail");
            return 0;
        });
    }

    // Wait and try again (transitions to half-open then closed)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto result = shield::circuit("test").run([]() { return 123; });

    REQUIRE(result == 123);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - half-open reopens on failure", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::milliseconds(50));
    
    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("test").run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) 
        {
        }
    }
    
    // Wait for half-open transition
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Fail again in half-open state
    try
    {
        shield::circuit("test").run([]()
        {
            throw std::runtime_error("Still failing");
            return 0;
        });
    }
    catch (...) {}
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - half-open reopens on failure with explicit exceptions", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::milliseconds(50));
    
    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        shield::circuit("test").run<std::runtime_error>([]()
        {
            throw ("Fail");
            return 0;
        });
    }
    
    // Wait for half-open transition
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Fail again in half-open state
    shield::circuit("test").run<std::runtime_error>([]()
    {
        throw std::runtime_error("Still failing");
        return 0;
    });
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - success resets failure count", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 5, std::chrono::seconds(1));
    
    // Fail a few times
    for (int i = 0; i < 3; i++)
    {
        try
        {
            shield::circuit("test").run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    REQUIRE(cb->get_failure_count() == 3);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    
    // Succeed once
    shield::circuit("test").run([]() { return 42; });
    
    REQUIRE(cb->get_failure_count() == 0);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - success resets failure count with explicit exception", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 5, std::chrono::seconds(1));
    
    // Fail a few times
    for (int i = 0; i < 3; i++)
    {
        shield::circuit("test").run<std::runtime_error>([]()
        {
            throw std::runtime_error("Fail");
            return 0;
        });
    }
    
    REQUIRE(cb->get_failure_count() == 3);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    
    // Succeed once
    shield::circuit("test").run([]() { return 42; });
    
    REQUIRE(cb->get_failure_count() == 0);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - custom failure threshold", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 10, std::chrono::seconds(1));
    
    // Fail 9 times - should still be closed
    for (int i = 0; i < 9; i++)
    {
        try
        {
            shield::circuit("test").run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    
    // 10th failure should open it
    try
    {
        shield::circuit("test").run([]()
        {
            throw std::runtime_error("Fail");
            return 0;
        });
    }
    catch (...) 
    {
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - custom failure threshold with explicit exception", "[circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 10, std::chrono::seconds(1));
    
    // Fail 9 times - should still be closed
    for (int i = 0; i < 9; i++)
    {
        shield::circuit("test").run<std::runtime_error>([]()
        {
            throw std::runtime_error("Fail");
            return 0;
        });
    }
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    
    // 10th failure should open it
    shield::circuit("test").run<std::runtime_error>([]()
    {
        throw std::runtime_error("Fail");
        return 0;
    });
    
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}