#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include <shield/all.hpp>

#include <iostream>

using namespace shield;

TEST_CASE("Retry policy - default configuration", "[retry_policy]")
{
    retry_policy policy;

    REQUIRE(policy.get_max_attempts() == 3);
    REQUIRE(policy.get_backoff_strategy() != nullptr);
}

TEST_CASE("Retry policy - custom max attempts", "[retry_policy]")
{
    retry_policy policy(5);

    REQUIRE(policy.get_max_attempts() == 5);
}

TEST_CASE("Retry policy - successful on first attempt", "[retry_policy]")
{
    retry_policy policy(3);
    int call_count = 0;

    auto result = policy.run([&call_count]()
    {
        call_count++;
        return 42;
    });

    REQUIRE(result == 42);
    REQUIRE(call_count == 1);
}

TEST_CASE("Retry policy - retries on failure", "[retry_policy]")
{
    retry_policy policy(5);
    int call_count = 0;

    auto result = policy.run([&call_count]()
    {
        call_count++;
        if (call_count < 3)
        {
            throw std::runtime_error("Temporary failure");
        }
        return 100;
    });

    REQUIRE(result == 100);
    REQUIRE(call_count == 3);
}

TEST_CASE("Retry policy - exhausts attempts", "[retry_policy]")
{
    retry_policy policy(3);
    int call_count = 0;

    REQUIRE_THROWS_AS(
        policy.run([&call_count]()
        {
            call_count++;
            throw std::runtime_error("Always fails");
            return 0;
        }),
        std::runtime_error
    );

    REQUIRE(call_count == 3);
}

// ============================================================================
// BUILDER PATTERN TESTS
// ============================================================================

TEST_CASE("Retry policy - builder pattern", "[retry_policy][builder]")
{
    auto policy = retry_policy()
        .with_max_attempts(5)
        .with_fixed_backoff(std::chrono::milliseconds(50));

    REQUIRE(policy.get_max_attempts() == 5);
}

TEST_CASE("Retry policy - method chaining", "[retry_policy][builder]")
{
    int call_count = 0;

    auto result = retry_policy()
        .with_max_attempts(4)
        .with_exponential_backoff(std::chrono::milliseconds(10))
        .run([&call_count]()
        {
            call_count++;
            if (call_count < 2)
            {
                throw std::runtime_error("Fail once");
            }
            return 99;
        });

    REQUIRE(result == 99);
    REQUIRE(call_count == 2);
}

// ============================================================================
// BACKOFF STRATEGY TESTS
// ============================================================================

TEST_CASE("Fixed backoff - constant delay", "[retry_policy][backoff]")
{
    fixed_backoff backoff(std::chrono::milliseconds(100));

    REQUIRE(backoff.calculate_delay(1) == std::chrono::milliseconds(100));
    REQUIRE(backoff.calculate_delay(2) == std::chrono::milliseconds(100));
    REQUIRE(backoff.calculate_delay(3) == std::chrono::milliseconds(100));
}

TEST_CASE("Exponential backoff - delay growth", "[retry_policy][backoff]")
{
    exponential_backoff backoff(
        std::chrono::milliseconds(100),
        2.0,
        std::chrono::seconds(10)
    );

    REQUIRE(backoff.calculate_delay(1) == std::chrono::milliseconds(100));
    REQUIRE(backoff.calculate_delay(2) == std::chrono::milliseconds(200));
    REQUIRE(backoff.calculate_delay(3) == std::chrono::milliseconds(400));
    REQUIRE(backoff.calculate_delay(4) == std::chrono::milliseconds(800));
}

TEST_CASE("Exponential backoff - respects max delay", "[retry_policy][backoff]")
{
    exponential_backoff backoff(
        std::chrono::milliseconds(100),
        2.0,
        std::chrono::milliseconds(500)
    );

    REQUIRE(backoff.calculate_delay(1) == std::chrono::milliseconds(100));
    REQUIRE(backoff.calculate_delay(2) == std::chrono::milliseconds(200));
    REQUIRE(backoff.calculate_delay(3) == std::chrono::milliseconds(400));
    REQUIRE(backoff.calculate_delay(4) == std::chrono::milliseconds(500));
    REQUIRE(backoff.calculate_delay(5) == std::chrono::milliseconds(500));
}

TEST_CASE("Linear backoff - linear growth", "[retry_policy][backoff]")
{
    linear_backoff backoff(
        std::chrono::milliseconds(50),
        std::chrono::seconds(1)
    );

    REQUIRE(backoff.calculate_delay(1) == std::chrono::milliseconds(50));
    REQUIRE(backoff.calculate_delay(2) == std::chrono::milliseconds(100));
    REQUIRE(backoff.calculate_delay(3) == std::chrono::milliseconds(150));
    REQUIRE(backoff.calculate_delay(4) == std::chrono::milliseconds(200));
}

TEST_CASE("Jittered backoff - adds randomness", "[retry_policy][backoff]")
{
    jittered_exponential_backoff backoff(
        std::chrono::milliseconds(100),
        2.0,
        std::chrono::seconds(10),
        0.2 // 20% jitter
    );

    auto delay1 = backoff.calculate_delay(1);
    auto delay2 = backoff.calculate_delay(1);

    // Should be different due to jitter (with high probability)
    // Base is 100ms, jitter is 20%, so range is 80-120ms
    REQUIRE(delay1.count() >= 80);
    REQUIRE(delay1.count() <= 120);
}

TEST_CASE("Retry policy - with fixed backoff", "[retry_policy][backoff]")
{
    auto start = std::chrono::steady_clock::now();
    int call_count = 0;

    try
    {
        retry_policy()
            .with_max_attempts(3)
            .with_fixed_backoff(std::chrono::milliseconds(50))
            .run([&call_count]()
            {
                call_count++;
                throw std::runtime_error("Fail");
                return 0;
            });
    }
    catch (...) {}

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should have 2 delays of 50ms each (after 1st and 2nd attempt)
    REQUIRE(duration.count() >= 100);
    REQUIRE(call_count == 3);
}

// ============================================================================
// EXCEPTION FILTERING TESTS
// ============================================================================

TEST_CASE("Retry policy - retry on specific exception", "[retry_policy][exceptions]")
{
    int call_count = 0;

    auto result = retry_policy(5)
        .retry_on<std::runtime_error>()
        .run([&call_count]()
        {
            call_count++;
            if (call_count < 2)
            {
                throw std::runtime_error("Retryable");
            }
            return 42;
        });

    REQUIRE(result == 42);
    REQUIRE(call_count == 2);
}

TEST_CASE("Retry policy - don't retry on non-matching exception", "[retry_policy][exceptions]")
{
    int call_count = 0;

    REQUIRE_THROWS_AS(
        retry_policy(5)
        .retry_on<std::runtime_error>()
        .run([&call_count]()
        {
            call_count++;
            throw std::logic_error("Not retryable");
            return 0;
        }),
        std::logic_error
    );

    REQUIRE(call_count == 1); // No retries
}

TEST_CASE("Retry policy - retry on multiple exception types", "[retry_policy][exceptions]")
{
    retry_policy policy(5);
    policy.retry_on<std::runtime_error>()
        .retry_on<std::logic_error>();

    int runtime_count = 0;
    policy.run([&runtime_count]()
    {
        runtime_count++;
        if (runtime_count < 2)
        {
            throw std::runtime_error("Retry this");
        }
        return 1;
    });
    REQUIRE(runtime_count == 2);

    int logic_count = 0;
    policy.run([&logic_count]()
    {
        logic_count++;
        if (logic_count < 2)
        {
            throw std::logic_error("Retry this too");
        }
        return 2;
    });
    REQUIRE(logic_count == 2);
}

TEST_CASE("Retry policy - custom retry predicate", "[retry_policy][exceptions]")
{
    int call_count = 0;

    auto result = retry_policy(5)
        .retry_if([](const std::exception& e, int attempt)
        {
            // Only retry on odd attempts
            return attempt % 2 == 1;
        })
        .run([&call_count]()
        {
            call_count++;
            if (call_count < 3)
            {
                throw std::runtime_error("Maybe retry");
            }
            return 100;
        });

    REQUIRE(result == 100);
}

// ============================================================================
// CALLBACK TESTS
// ============================================================================

TEST_CASE("Retry policy - on_retry callback", "[retry_policy][callback]")
{
    int callback_count = 0;
    std::vector<int> attempts;

    retry_policy policy(4);
    policy.on_retry([&](const std::exception& e, int attempt,
        std::chrono::milliseconds delay)
        {
            callback_count++;
            attempts.push_back(attempt);
        });

    int call_count = 0;
    try
    {
        policy.run([&call_count]()
        {
            call_count++;
            throw std::runtime_error("Fail");
            return 0;
        });
    }
    catch (...) {}

    REQUIRE(callback_count == 3); // 3 retries before final failure
    REQUIRE(attempts == std::vector<int>{1, 2, 3});
}

// ============================================================================
// COPY AND MOVE TESTS
// ============================================================================

TEST_CASE("Retry policy - copy constructor", "[retry_policy][copy]")
{
    retry_policy policy1(5);
    policy1.with_fixed_backoff(std::chrono::milliseconds(100));

    retry_policy policy2(policy1);

    REQUIRE(policy2.get_max_attempts() == 5);
    REQUIRE(policy2.get_backoff_strategy() != nullptr);
}

TEST_CASE("Retry policy - copy assignment", "[retry_policy][copy]")
{
    retry_policy policy1(5);
    policy1.with_exponential_backoff(std::chrono::milliseconds(50));

    retry_policy policy2(3);
    policy2 = policy1;

    REQUIRE(policy2.get_max_attempts() == 5);
}

TEST_CASE("Retry policy - move semantics", "[retry_policy][move]")
{
    retry_policy policy1(7);
    policy1.with_linear_backoff(std::chrono::milliseconds(25));

    retry_policy policy2(std::move(policy1));

    REQUIRE(policy2.get_max_attempts() == 7);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_CASE("Retry policy - complex real-world scenario", "[retry_policy][integration]")
{
    int total_attempts = 0;
    int callback_count = 0;
    bool succeeded = false;

    auto policy = retry_policy()
        .with_max_attempts(5)
        .with_jittered_backoff(std::chrono::milliseconds(10))
        .retry_on<std::runtime_error>()
        .on_retry([&](const std::exception&, int, std::chrono::milliseconds)
        {
            callback_count++;
        });

    auto result = policy.run([&]()
    {
        total_attempts++;
        if (total_attempts < 3)
        {
            throw std::runtime_error("Transient failure");
        }
        succeeded = true;
        return std::string("Success");
    });

    REQUIRE(result == "Success");
    REQUIRE(total_attempts == 3);
    REQUIRE(callback_count == 2);
    REQUIRE(succeeded);
}

TEST_CASE("Retry policy - convenience functions", "[retry_policy][convenience]")
{
    auto policy1 = make_retry_policy(4);
    REQUIRE(policy1.get_max_attempts() == 4);

    auto policy2 = make_exponential_retry_policy(3, std::chrono::milliseconds(50));
    REQUIRE(policy2.get_max_attempts() == 3);

    auto policy3 = make_jittered_retry_policy(2, std::chrono::milliseconds(100));
    REQUIRE(policy3.get_max_attempts() == 2);
}

TEST_CASE("Retry policy - void return type", "[retry_policy]")
{
    int call_count = 0;
    bool executed = false;

    retry_policy(3).run([&]()
    {
        call_count++;
        if (call_count < 2)
        {
            throw std::runtime_error("Fail once");
        }
        executed = true;
    });

    REQUIRE(call_count == 2);
    REQUIRE(executed);
}

TEST_CASE("Retry policy - different return types", "[retry_policy]")
{
    // String
    auto str_result = retry_policy(3).run([]()
    {
        return std::string("test");
    });
    REQUIRE(str_result == "test");

    // Double
    auto double_result = retry_policy(3).run([]()
    {
        return 3.14159;
    });
    REQUIRE(double_result == 3.14159);

    // Struct
    struct result { int code; };
    auto struct_result = retry_policy(3).run([]()
    {
        return result{ 42 };
    });
    REQUIRE(struct_result.code == 42);
}