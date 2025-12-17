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

// ============================================================================
// FALLBACK INTEGRATION TESTS - Circuit Breaker Open State
// ============================================================================

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - returns fallback value when circuit is open", "[circuit][fallback]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("fallback-test", 2, std::chrono::seconds(10));

    shield::fallback_policy fallback = shield::fallback_policy::with_value(999);

    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("fallback-test", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Circuit is open - should return fallback value
    int result = shield::circuit("fallback-test", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]()
    {
        return 42; // This won't execute
    });

    REQUIRE(result == 999);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - returns fallback string when circuit is open", "[circuit][fallback]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("string-fallback-test", 2, std::chrono::seconds(10));

    shield::fallback_policy fallback = shield::fallback_policy::with_value(std::string("fallback response"));

    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("string-fallback-test", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]()
            {
                throw std::runtime_error("Fail");
                return std::string();
            });
        }
        catch (...) {}
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Circuit is open - should return fallback value
    std::string result = shield::circuit("string-fallback-test", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]()
    {
        return std::string("real response");
    });

    REQUIRE(result == "fallback response");
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - callable fallback invoked when circuit is open", "[circuit][fallback][ctor]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("callable-fallback", 2, std::chrono::seconds(10));

    int fallback_call_count = 0;
    shield::fallback_policy fallback = shield::fallback_policy::with_typed_callable([&fallback_call_count]()
    {
        ++fallback_call_count;
        return 888;
    });

    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("callable-fallback", std::nullopt, std::nullopt, fallback).run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    REQUIRE(fallback_call_count == 0);

    // Circuit is open - should invoke callable fallback
    int result = shield::circuit("callable-fallback", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]()
    {
        return 42;
    });

    REQUIRE(result == 888);
    REQUIRE(fallback_call_count == 1);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - callable fallback invoked when circuit is open", "[circuit][fallback][builder]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("callable-fallback", 2, std::chrono::seconds(10));

    int fallback_call_count = 0;
    shield::fallback_policy fallback = shield::fallback_policy::with_typed_callable([&fallback_call_count]()
    {
        ++fallback_call_count;
        return 888;
    });

    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("callable-fallback")
                .with_fallback_policy(fallback)
                .run([]()
                {
                    throw std::runtime_error("Fail");
                    return 0;
                });
        }
        catch (...) {}
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    REQUIRE(fallback_call_count == 0);

    // Circuit is open - should invoke callable fallback
    int result = shield::circuit("callable-fallback", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]()
    {
        return 42;
    });

    REQUIRE(result == 888);
    REQUIRE(fallback_call_count == 1);
}


TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - default fallback returns default-constructed value when circuit is open", "[circuit][fallback]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("default-fallback", 2, std::chrono::seconds(10));

    shield::fallback_policy fallback = shield::fallback_policy::with_default();

    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("default-fallback", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Circuit is open - should return default-constructed int (0)
    int result = shield::circuit("default-fallback", std::nullopt, std::nullopt, fallback).run([]()
    {
        return 42;
    });

    REQUIRE(result == 0);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - throw fallback throws when circuit is open", "[circuit][fallback]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("throw-fallback", 2, std::chrono::seconds(10));

    shield::fallback_policy fallback = shield::fallback_policy::with_throw();

    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("throw-fallback", std::nullopt, std::nullopt, fallback).run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Circuit is open with throw fallback - should throw
    REQUIRE_THROWS_AS(
        shield::circuit("throw-fallback", std::nullopt, std::nullopt, fallback).run([]()
        {
            return 42;
        }),
        shield::fallback_exception
    );
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - no fallback throws open_circuit_exception when circuit is open", "[circuit][fallback]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("no-fallback", 2, std::chrono::seconds(10));

    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit("no-fallback").run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Circuit is open without fallback - should throw open_circuit_exception
    REQUIRE_THROWS_AS(
        shield::circuit("no-fallback").run([]() { return 42; }),
        shield::open_circuit_exception
    );
}

// ============================================================================
// FALLBACK INTEGRATION TESTS - Exception Handling
// ============================================================================

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - returns fallback value on exception", "[circuit][fallback][exception][ctor]")
{
    shield::fallback_policy fallback = shield::fallback_policy::with_value(777);

    int result = shield::circuit("exception-fallback", std::nullopt, std::nullopt, fallback).run<std::runtime_error>([]()
    {
        throw std::runtime_error("Operation failed");
        return 42;
    });

    REQUIRE(result == 777);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - returns fallback value on exception", "[circuit][fallback][exception][builder]")
{
    shield::fallback_policy fallback = shield::fallback_policy::with_value(777);

    int result = shield::circuit("exception-fallback")
        .with_fallback_policy(fallback)
        .run<std::runtime_error>([]()
        {
            throw std::runtime_error("Operation failed");
            return 42;
        });

    REQUIRE(result == 777);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - callable fallback invoked on exception", "[circuit][fallback][exception][ctor]")
{
    int fallback_invocations = 0;
    shield::fallback_policy fallback = shield::fallback_policy::with_typed_callable([&fallback_invocations]()
    {
        ++fallback_invocations;
        return 555;
    });

    int result = shield::circuit("exception-callable", std::nullopt, std::nullopt, fallback)
        .run<std::runtime_error>([]()
        {
            throw std::runtime_error("Fail");
            return 42;
        });

    REQUIRE(result == 555);
    REQUIRE(fallback_invocations == 1);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - callable fallback invoked on exception", "[circuit][fallback][exception][builder]")
{
    int fallback_invocations = 0;
    shield::fallback_policy fallback = shield::fallback_policy::with_typed_callable([&fallback_invocations]()
    {
        ++fallback_invocations;
        return 555;
    });

    int result = shield::circuit("exception-callable")
        .with_fallback_policy(fallback)
        .run<std::runtime_error>([]()
        {
            throw std::runtime_error("Fail");
            return 42;
        });

    REQUIRE(result == 555);
    REQUIRE(fallback_invocations == 1);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - fallback value takes precedence over default construction on exception", "[circuit][fallback][exception]")
{
    shield::fallback_policy fallback = shield::fallback_policy::with_value(123);

    int result = shield::circuit("precedence-test", std::nullopt, std::nullopt, fallback).run<std::runtime_error>([]()
    {
        throw std::runtime_error("Fail");
        return 42;
    });

    // Should return fallback value (123), not default-constructed value (0)
    REQUIRE(result == 123);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - throw fallback rethrows exception", "[circuit][fallback][exception]")
{
    shield::fallback_policy fallback = shield::fallback_policy::with_throw();

    REQUIRE_THROWS_AS(
        shield::circuit("throw-on-exception", std::nullopt, std::nullopt, fallback).run<std::runtime_error>([]()
        {
            throw std::runtime_error("Original error");
            return 42;
        }),
        shield::fallback_exception
    );
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - void function with fallback does not throw", "[circuit][fallback][exception]")
{
    shield::fallback_policy fallback = shield::fallback_policy::with_default();

    // Should not throw - void functions consume exceptions with fallback
    REQUIRE_NOTHROW(
        shield::circuit("void-fallback", std::nullopt, std::nullopt, fallback).run<std::runtime_error>([]()
        {
            throw std::runtime_error("Fail");
        })
    );
}

// ============================================================================
// FALLBACK INTEGRATION TESTS - Complex Scenarios
// ============================================================================

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - fallback with retry policy", "[circuit][retry_policy][fallback][ctor]")
{
    int attempt_count = 0;
    int fallback_count = 0;

    shield::retry_policy retry = shield::retry_policy()
        .with_max_attempts(3)
        .with_fixed_backoff(std::chrono::milliseconds(5));

    shield::fallback_policy fallback = shield::fallback_policy::with_typed_callable([&fallback_count]()
    {
        ++fallback_count;
        return 999;
    });

    int result = shield::circuit("fallback-with-retry", retry, std::nullopt, fallback)
        .run<std::runtime_error>([&attempt_count]()
        {
            ++attempt_count;
            throw std::runtime_error("Always fails");
            return 42;
        });

    // Should retry 3 times, then return fallback
    REQUIRE(attempt_count == 3);
    REQUIRE(fallback_count == 1);
    REQUIRE(result == 999);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - fallback with retry policy", "[circuit][retry_policy][fallback][builder]")
{
    int attempt_count = 0;
    int fallback_count = 0;

    shield::retry_policy retry = shield::retry_policy()
        .with_max_attempts(3)
        .with_fixed_backoff(std::chrono::milliseconds(5));

    shield::fallback_policy fallback = shield::fallback_policy::with_typed_callable([&fallback_count]()
    {
        ++fallback_count;
        return 999;
    });

    int result = shield::circuit("fallback-with-retry")
        .with_retry_policy(retry)
        .with_fallback_policy(fallback)
        .run<std::runtime_error>([&attempt_count]()
        {
            ++attempt_count;
            throw std::runtime_error("Always fails");
            return 42;
        });

    // Should retry 3 times, then return fallback
    REQUIRE(attempt_count == 3);
    REQUIRE(fallback_count == 1);
    REQUIRE(result == 999);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - different fallbacks for different circuits", "[circuit][fallback]")
{
    shield::fallback_policy fallback1 = shield::fallback_policy::with_value(100);
    shield::fallback_policy fallback2 = shield::fallback_policy::with_value(200);

    std::shared_ptr<shield::circuit_breaker> cb1 = shield::circuit_breaker::create("circuit1", 1, std::chrono::seconds(10));
    std::shared_ptr<shield::circuit_breaker> cb2 = shield::circuit_breaker::create("circuit2", 1, std::chrono::seconds(10));

    // Open both circuits
    try
    {
        shield::circuit("circuit1", shield::default_retry_policy, shield::default_timeout_policy, fallback1).run([]()
        {
            throw std::runtime_error("Fail");
            return 0;
        });
    }
    catch (...) {}

    try
    {
        shield::circuit("circuit2", shield::default_retry_policy, shield::default_timeout_policy, fallback2).run([]()
        {
            throw std::runtime_error("Fail");
            return 0;
        });
    }
    catch (...) {}

    REQUIRE(cb1->get_state() == shield::circuit_breaker::state::open);
    REQUIRE(cb2->get_state() == shield::circuit_breaker::state::open);

    // Each circuit should use its own fallback
    int result1 = shield::circuit("circuit1", shield::default_retry_policy, shield::default_timeout_policy, fallback1).run([]() { return 0; });
    int result2 = shield::circuit("circuit2", shield::default_retry_policy, shield::default_timeout_policy, fallback2).run([]() { return 0; });

    REQUIRE(result1 == 100);
    REQUIRE(result2 == 200);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - static run method with fallback", "[circuit][fallback]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("static-fallback", 2, std::chrono::seconds(10));

    shield::fallback_policy fallback = shield::fallback_policy::with_value(444);

    // Open circuit
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit::run(
                []() { throw std::runtime_error("Fail"); return 0; },
                "static-fallback",
                shield::default_retry_policy,
                shield::default_timeout_policy,
                fallback
            );
        }
        catch (...) {}
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Should return fallback
    int result = shield::circuit::run(
        []() { return 42; },
        "static-fallback",
        shield::default_retry_policy,
        shield::default_timeout_policy,
        fallback
    );

    REQUIRE(result == 444);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - complex struct as fallback value", "[circuit][fallback]")
{
    struct Response
    {
        int code;
        std::string message;
        bool success;
    };

    Response fallback_response{503, "Service unavailable", false};
    shield::fallback_policy fallback = shield::fallback_policy::with_value(fallback_response);

    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("struct-fallback", 1, std::chrono::seconds(10));

    // Open circuit
    try
    {
        shield::circuit("struct-fallback", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]() -> Response
        {
            throw std::runtime_error("Fail");
        });
    }
    catch (...) {}

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Should return fallback struct
    Response result = shield::circuit("struct-fallback", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]() -> Response
    {
        return {200, "OK", true};
    });

    REQUIRE(result.code == 503);
    REQUIRE(result.message == "Service unavailable");
    REQUIRE(result.success == false);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with fallback - fallback with context capture", "[circuit][fallback]")
{
    std::string service_name = "PaymentService";
    int retry_after_seconds = 60;

    shield::fallback_policy fallback = shield::fallback_policy::with_typed_callable([service_name, retry_after_seconds]()
    {
        return service_name + " unavailable, retry after " + std::to_string(retry_after_seconds) + " seconds";
    });

    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("context-fallback", 1, std::chrono::seconds(10));

    // Open circuit
    try
    {
        shield::circuit("context-fallback", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]()
        {
            throw std::runtime_error("Fail");
            return std::string();
        });
    }
    catch (...) {}

    std::string result = shield::circuit("context-fallback", shield::default_retry_policy, shield::default_timeout_policy, fallback).run([]()
    {
        return std::string("success");
    });

    REQUIRE(result == "PaymentService unavailable, retry after 60 seconds");
}

// ============================================================================
// ORIGINAL CIRCUIT TESTS (WITHOUT RETRY POLICY)
// ============================================================================

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - initial state is closed", "[circuit][circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 3, std::chrono::seconds(1));

    shield::circuit("test").run([]
    {
        return true;
    });

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - successful execution", "[circuit][circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 3, std::chrono::seconds(1));

    const int result = shield::circuit("test").run([]()
    {
        return 42;
    });

    REQUIRE(result == 42);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - opens after threshold failures", "[circuit][circuit_breaker]")
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
        catch (const std::exception&)
        {
            // Expected
        }
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
    REQUIRE(cb->get_failure_count() == 3);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - rejects calls when open", "[circuit][circuit_breaker]")
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
    REQUIRE_THROWS_AS(
        shield::circuit("test").run([]() { return 42; }),
        shield::open_circuit_exception
    );
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - transitions to half-open after timeout", "[circuit][circuit_breaker]")
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
    const int result = shield::circuit("test").run([]() { return 99; });

    REQUIRE(result == 99);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - transitions to half-open after timeout with explicit exception", "[circuit][circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::milliseconds(1000));

    // Open the circuit
    for (int i = 0; i < 2; i++)
    {
        shield::circuit("test").run<std::runtime_error>([]()
        {
            throw std::runtime_error("Fail");
            return 0;
        });
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Next call should transition to half-open
    const int result = shield::circuit("test").run([]() { return 99; });

    REQUIRE(result == 99);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb->get_failure_count() == 0);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - half-open closes on success", "[circuit][circuit_breaker]")
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

    const int result = shield::circuit("test").run([]() { return 123; });

    REQUIRE(result == 123);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - half-open closes on success with explicit exception", "[circuit][circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::milliseconds(1000));

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
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    const int result = shield::circuit("test").run([]() { return 123; });

    REQUIRE(result == 123);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - half-open reopens on failure", "[circuit][circuit_breaker]")
{
    std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("test", 2, std::chrono::milliseconds(1000));

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
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

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

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - half-open reopens on failure with explicit exceptions", "[circuit][circuit_breaker]")
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

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - success resets failure count", "[circuit][circuit_breaker]")
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

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - success resets failure count with explicit exception", "[circuit][circuit_breaker]")
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

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - custom failure threshold", "[circuit][circuit_breaker]")
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
        catch (...) 
        {
        }
    }

    REQUIRE(cb->get_failure_count() == 9);
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

    REQUIRE(cb->get_failure_count() == 10);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit - custom failure threshold with explicit exception", "[circuit][circuit_breaker]")
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

    REQUIRE(cb->get_failure_count() == 9);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::closed);

    // 10th failure should open it
    shield::circuit("test").run<std::runtime_error>([]()
    {
        throw std::runtime_error("Fail");
        return 0;
    });

    REQUIRE(cb->get_failure_count() == 10);
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}

// ============================================================================
// CIRCUIT WITH RETRY POLICY TESTS
// ============================================================================

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - basic integration", "[circuit][retry_policy]")
{
    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(3)
        .with_fixed_backoff(std::chrono::milliseconds(10));

    int call_count = 0;

    const std::string result = shield::circuit("test-basic")
        .with_retry_policy(policy)
        .run<std::runtime_error>([&call_count]()
        {
            ++call_count;
            if (call_count < 2)
            {
                throw std::runtime_error("Temporary failure");
            }
            return std::string("success");
        });

    REQUIRE(result == "success");
    REQUIRE(call_count == 2);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - exhausts retries", "[circuit][retry_policy]")
{
    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(3)
        .with_fixed_backoff(std::chrono::milliseconds(10));

    int call_count = 0;

    REQUIRE_THROWS_AS(
        shield::circuit("test-exhaust")
        .with_retry_policy(policy)
        .run<std::runtime_error>([&call_count]()
        {
            ++call_count;
            throw std::runtime_error("Always fails");
            return std::string("never");
        }),
        std::runtime_error
    );

    REQUIRE(call_count == 3);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - exponential backoff", "[circuit][retry_policy]")
{
    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(4)
        .with_exponential_backoff(std::chrono::milliseconds(10), 2.0, std::chrono::seconds(1));

    int call_count = 0;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    try
    {
        shield::circuit("test-exponential")
            .with_retry_policy(policy)
            .run<std::runtime_error>([&call_count]()
            {
                ++call_count;
                throw std::runtime_error("Fail");
                return 0;
            });
    }
    catch (...)
    {
        // Expected
    }

    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    REQUIRE(call_count == 4);
    // Total delay should be approximately: 10 + 20 + 40 = 70ms minimum
    REQUIRE(duration.count() >= 60);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - specific exception filtering", "[circuit][retry_policy]")
{
    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(5)
        .retry_on<std::runtime_error>()
        .with_fixed_backoff(std::chrono::milliseconds(5));

    int runtime_count = 0;

    // Should retry on runtime_error
    const std::string result1 = shield::circuit("test-filter-1")
        .with_retry_policy(policy)
        .run<std::runtime_error>([&runtime_count]()
        {
            ++runtime_count;
            if (runtime_count < 2)
            {
                throw std::runtime_error("Retry this");
            }
            return std::string("ok");
        });

    REQUIRE(result1 == "ok");
    REQUIRE(runtime_count == 2);

    int logic_count = 0;

    // Should NOT retry on logic_error (not in retry list)
    REQUIRE_THROWS_AS(
        shield::circuit("test-filter-2")
        .with_retry_policy(policy)
        .run<std::logic_error>([&logic_count]()
        {
            ++logic_count;
            throw std::logic_error("Don't retry this");
            return std::string("never");
        }),
        shield::cannot_obtain_value_exception
    );

    REQUIRE(logic_count == 1); // No retries
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - custom predicate", "[circuit][retry_policy]")
{
    int predicate_calls = 0;

    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(4)
        .retry_if([&predicate_calls](const std::exception& e, int attempt)
        {
            ++predicate_calls;
            // Only retry on odd attempts
            return attempt % 2 == 1;
        })
        .with_fixed_backoff(std::chrono::milliseconds(5));

    int call_count = 0;

    REQUIRE_THROWS_AS(
        shield::circuit("test-predicate")
            .with_retry_policy(policy)
            .run<std::runtime_error>([&call_count]()
            {
                ++call_count;
                if (call_count < 3)
                {
                    throw std::runtime_error("Maybe retry");
                }
                return std::string("done");
            }),
        std::runtime_error
    );

    REQUIRE(predicate_calls == 2);
    REQUIRE(call_count == 2);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - callback invocation", "[circuit][retry_policy]")
{
    int callback_count = 0;
    std::vector<int> retry_attempts;

    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(4)
        .with_fixed_backoff(std::chrono::milliseconds(5))
        .on_retry([&](const std::exception& e, int attempt, std::chrono::milliseconds delay)
        {
            ++callback_count;
            retry_attempts.push_back(attempt);
        });

    int call_count = 0;

    try
    {
        shield::circuit("test-callback")
            .with_retry_policy(policy)
            .run<std::runtime_error>([&call_count]()
            {
                ++call_count;
                throw std::runtime_error("Always fails");
                return 0;
            });
    }
    catch (...)
    {
        // Expected
    }

    REQUIRE(callback_count == 3); // 3 retries before final failure
    REQUIRE(retry_attempts == std::vector<int>{1, 2, 3});
    REQUIRE(call_count == 4);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - void return type", "[circuit][retry_policy]")
{
    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(3)
        .with_fixed_backoff(std::chrono::milliseconds(5));

    int call_count = 0;
    bool executed = false;

    shield::circuit("test-void")
        .with_retry_policy(policy)
        .run<std::runtime_error>([&]()
        {
            ++call_count;
            if (call_count < 2)
            {
                throw std::runtime_error("Fail once");
            }
            executed = true;
        });

    REQUIRE(call_count == 2);
    REQUIRE(executed);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - int return type", "[circuit][retry_policy]")
{
    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(3)
        .with_fixed_backoff(std::chrono::milliseconds(5));

    int call_count = 0;

    const int result = shield::circuit("test-int")
        .with_retry_policy(policy)
        .run<std::runtime_error>([&call_count]()
        {
            ++call_count;
            if (call_count < 2)
            {
                throw std::runtime_error("Fail once");
            }
            return 42;
        });

    REQUIRE(result == 42);
    REQUIRE(call_count == 2);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - builder pattern chaining", "[circuit][retry_policy]")
{
    int call_count = 0;

    std::string result = shield::circuit("test-builder")
        .with_retry_policy(
            shield::retry_policy()
            .with_max_attempts(3)
            .with_exponential_backoff(std::chrono::milliseconds(5))
        )
        .run<std::runtime_error>([&call_count]()
        {
            ++call_count;
            if (call_count < 2)
            {
                throw std::runtime_error("Try again");
            }
            return std::string("success");
        });

    REQUIRE(result == "success");
    REQUIRE(call_count == 2);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - circuit breaker interaction", "[circuit][retry_policy]")
{
    std::shared_ptr<shield::circuit_breaker> cb =
        shield::circuit_breaker::create("cb-test", 6, std::chrono::seconds(10));

    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(2)
        .with_fixed_backoff(std::chrono::milliseconds(5));

    // Make multiple failing calls
    // Each call with 2 attempts = 2 failures per call
    // Need 3 calls to reach 6 failures threshold
    for (int i = 0; i < 3; i++)
    {
        try
        {
            shield::circuit(cb)
                .with_retry_policy(policy)
                .run<std::runtime_error>([]()
                {
                    throw std::runtime_error("Fail");
                    return 0;
                });
        }
        catch (...)
        {
            // Expected
        }
    }

    // Circuit should be open after threshold failures
    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - jittered backoff randomness", "[circuit][retry_policy]")
{
    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(3)
        .with_jittered_backoff(
            std::chrono::milliseconds(50),
            2.0,
            std::chrono::seconds(5),
            0.2
        );

    int call_count = 0;

    try
    {
        shield::circuit("test-jitter")
            .with_retry_policy(policy)
            .run<std::runtime_error>([&call_count]()
            {
                ++call_count;
                throw std::runtime_error("Fail");
                return 0;
            });
    }
    catch (...)
    {
        // Expected
    }

    REQUIRE(call_count == 3);
    // Just verify it executed - the jitter will vary the timing
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - multiple circuits different policies", "[circuit][retry_policy]")
{
    shield::retry_policy fast_policy = shield::retry_policy()
        .with_max_attempts(2)
        .with_fixed_backoff(std::chrono::milliseconds(5));

    shield::retry_policy slow_policy = shield::retry_policy()
        .with_max_attempts(4)
        .with_fixed_backoff(std::chrono::milliseconds(10));

    int fast_count = 0;
    int slow_count = 0;

    const std::string result1 = shield::circuit("fast-circuit")
        .with_retry_policy(fast_policy)
        .run<std::runtime_error>([&fast_count]()
        {
            ++fast_count;
            if (fast_count < 2)
            {
                throw std::runtime_error("Retry");
            }
            return std::string("fast");
        });

    const std::string result2 = shield::circuit("slow-circuit")
        .with_retry_policy(slow_policy)
        .run<std::runtime_error>([&slow_count]()
        {
            ++slow_count;
            if (slow_count < 3)
            {
                throw std::runtime_error("Retry");
            }
            return std::string("slow");
        });

    REQUIRE(result1 == "fast");
    REQUIRE(result2 == "slow");
    REQUIRE(fast_count == 2);
    REQUIRE(slow_count == 3);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - convenience functions", "[circuit][retry_policy]")
{
    shield::retry_policy policy = shield::make_exponential_retry_policy(3, std::chrono::milliseconds(5));

    int call_count = 0;

    const int result = shield::circuit("convenience-test")
        .with_retry_policy(policy)
        .run<std::runtime_error>([&call_count]()
        {
            ++call_count;
            if (call_count < 2)
            {
                throw std::runtime_error("Fail");
            }
            return 100;
        });

    REQUIRE(result == 100);
    REQUIRE(call_count == 2);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - linear backoff", "[circuit][retry_policy]")
{
    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(4)
        .with_linear_backoff(std::chrono::milliseconds(10), std::chrono::seconds(1));

    int call_count = 0;
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    try
    {
        shield::circuit("test-linear")
            .with_retry_policy(policy)
            .run<std::runtime_error>([&call_count]()
            {
                ++call_count;
                throw std::runtime_error("Fail");
                return 0;
            });
    }
    catch (...)
    {
        // Expected
    }

    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    const std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    REQUIRE(call_count == 4);
    // Linear delays: 10 + 20 + 30 = 60ms minimum
    REQUIRE(duration.count() >= 55);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit without retry policy - normal operation", "[circuit][retry_policy]")
{
    int call_count = 0;

    // Circuit without retry policy should work normally
    const std::string result = shield::circuit("no-retry")
        .run<std::runtime_error>([&call_count]()
        {
            ++call_count;
            return std::string("no-retry-needed");
        });

    REQUIRE(result == "no-retry-needed");
    REQUIRE(call_count == 1);
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit without retry policy - fails immediately", "[circuit][retry_policy]")
{
    int call_count = 0;

    const std::string returnValue = shield::circuit("fail-fast")
        .run<std::runtime_error>([&call_count]()
        {
            ++call_count;
            throw std::runtime_error("Immediate failure");
            return std::string("never");
        });
    
    REQUIRE(returnValue.empty());
    REQUIRE(call_count == 1); // No retries
}

TEST_CASE_METHOD(circuit_test_fixture, "Circuit with retry policy - respects circuit open state", "[circuit][retry_policy]")
{
    const std::shared_ptr<shield::circuit_breaker> cb = shield::circuit_breaker::create("open-test", 2, std::chrono::seconds(10));

    shield::retry_policy policy = shield::retry_policy()
        .with_max_attempts(3)
        .with_fixed_backoff(std::chrono::milliseconds(5));

    // Open the circuit first
    for (int i = 0; i < 2; i++)
    {
        try
        {
            shield::circuit(cb).run([]()
            {
                throw std::runtime_error("Fail");
                return 0;
            });
        }
        catch (...) {}
    }

    REQUIRE(cb->get_state() == shield::circuit_breaker::state::open);

    // Even with retry policy, circuit should reject when open
    REQUIRE_THROWS_WITH(
        shield::circuit(cb)
        .with_retry_policy(policy)
        .run<std::runtime_error>([]() { return 42; }),
        "Circuit is OPEN and no fallback value could be obtained."
    );
}