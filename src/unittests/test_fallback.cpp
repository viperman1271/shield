#include <shield/fallback.hpp>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <map>
#include <random>
#include <string>
#include <vector>

struct ServiceResponse
{
    int status_code;
    std::string message;
    bool success;

    bool operator==(const ServiceResponse& other) const
    {
        return status_code == other.status_code && message == other.message && success == other.success;
    }
};

TEST_CASE("fallback_policy - DEFAULT type returns default-constructed int", "[fallback_policy][default]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();
    const std::optional<int> result = policy.get_value<int>();

    REQUIRE(result.has_value());
    REQUIRE(*result == 0);
}

TEST_CASE("fallback_policy - DEFAULT type returns default-constructed string", "[fallback_policy][default]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();
    const std::optional<std::string> result = policy.get_value<std::string>();

    REQUIRE(result.has_value());
    REQUIRE(result->empty());
}

TEST_CASE("fallback_policy - DEFAULT type returns default-constructed vector", "[fallback_policy][default]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();
    const std::optional<std::vector<int>> result = policy.get_value<std::vector<int>>();

    REQUIRE(result.has_value());
    REQUIRE(result->empty());
}

TEST_CASE("fallback_policy - DEFAULT type has correct policy type", "[fallback_policy][default]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();

    REQUIRE(policy.get_type() == shield::fallback_type::DEFAULT);
}

TEST_CASE("fallback_policy - DEFAULT type does not have specific value", "[fallback_policy][default]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();

    REQUIRE_FALSE(policy.has_specific_value());
}

TEST_CASE("fallback_policy - DEFAULT type does not have callable", "[fallback_policy][default]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();

    REQUIRE_FALSE(policy.has_callable());
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE stores and retrieves integer value", "[fallback_policy][specific_value]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);
    const std::optional<int> result = policy.get_value<int>();

    REQUIRE(result.has_value());
    REQUIRE(*result == 42);
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE stores and retrieves string value", "[fallback_policy][specific_value]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(std::string("Circuit breaker activated!"));
    const std::optional<std::string> result = policy.get_value<std::string>();

    REQUIRE(result.has_value());
    REQUIRE(*result == "Circuit breaker activated!");
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE stores and retrieves double value", "[fallback_policy][specific_value]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(3.14159);
    const std::optional<double> result = policy.get_value<double>();

    REQUIRE(result.has_value());
    REQUIRE(*result == 3.14159);
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE stores and retrieves vector", "[fallback_policy][specific_value]")
{
    std::vector<int> expected{ 1, 2, 3, 4, 5 };
    shield::fallback_policy policy = shield::fallback_policy::with_value(expected);
    std::optional<std::vector<int>> result = policy.get_value<std::vector<int>>();

    REQUIRE(result.has_value());
    REQUIRE(*result == expected);
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE stores and retrieves map", "[fallback_policy][specific_value]")
{
    std::map<std::string, int> expected{ {"error_code", 503}, {"retry_after", 60} };
    shield::fallback_policy policy = shield::fallback_policy::with_value(expected);
    auto result = policy.get_value<std::map<std::string, int>>();

    REQUIRE(result.has_value());
    REQUIRE(*result == expected);
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE stores and retrieves custom struct", "[fallback_policy][specific_value]")
{
    ServiceResponse expected{ 503, "Service unavailable", false };
    shield::fallback_policy policy = shield::fallback_policy::with_value(expected);
    const std::optional<ServiceResponse> result = policy.get_value<ServiceResponse>();

    REQUIRE(result.has_value());
    REQUIRE(*result == expected);
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE wrong type returns nullopt", "[fallback_policy][specific_value]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);
    const std::optional<std::string> result = policy.get_value<std::string>();

    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE has correct policy type", "[fallback_policy][specific_value]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);

    REQUIRE(policy.get_type() == shield::fallback_type::SPECIFIC_VALUE);
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE has specific value", "[fallback_policy][specific_value]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);

    REQUIRE(policy.has_specific_value());
}

TEST_CASE("fallback_policy - SPECIFIC_VALUE does not have callable", "[fallback_policy][specific_value]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);

    REQUIRE_FALSE(policy.has_callable());
}

TEST_CASE("fallback_policy - CALLABLE executes lambda returning string", "[fallback_policy][callable]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([]() { return std::string("Fallback from lambda"); });
    const std::optional<std::string> result = policy.get_value<std::string>();

    REQUIRE(result.has_value());
    REQUIRE(*result == "Fallback from lambda");
}

TEST_CASE("fallback_policy - CALLABLE executes lambda returning int", "[fallback_policy][callable]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([]() 
    { 
        return 12345; 
    });
    const std::optional<int> result = policy.get_value<int>();

    REQUIRE(result.has_value());
    REQUIRE(*result == 12345);
}

TEST_CASE("fallback_policy - CALLABLE executes lambda with capture", "[fallback_policy][callable]")
{
    int error_count = 5;
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([error_count]() 
    { 
        return std::string("Error occurred " + std::to_string(error_count) + " times"); 
    });
    const std::optional<std::string> result = policy.get_value<std::string>();

    REQUIRE(result.has_value());
    REQUIRE(*result == "Error occurred 5 times");
}

TEST_CASE("fallback_policy - CALLABLE executes lambda returning complex type", "[fallback_policy][callable]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([]() 
    { 
        return std::map<std::string, int>{{"error_code", 503}, { "retry_after", 60 }}; 
    });
    auto result = policy.get_value<std::map<std::string, int>>();

    REQUIRE(result.has_value());
    REQUIRE(result->at("error_code") == 503);
    REQUIRE(result->at("retry_after") == 60);
}

TEST_CASE("fallback_policy - CALLABLE has correct policy type", "[fallback_policy][callable]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([]() { return std::string("test"); });

    REQUIRE(policy.get_type() == shield::fallback_type::CALLABLE);
}

TEST_CASE("fallback_policy - CALLABLE has callable", "[fallback_policy][callable]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([]() { return std::string("test"); });

    REQUIRE(policy.has_callable());
}

TEST_CASE("fallback_policy - CALLABLE does not have specific value", "[fallback_policy][callable]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([]() { return std::string("test"); });

    REQUIRE_FALSE(policy.has_specific_value());
}

TEST_CASE("fallback_policy - CALLABLE with manual std::any returning int", "[fallback_policy][callable]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_callable([]() -> std::any { return 999; });
    const std::optional<int> result = policy.get_value<int>();

    REQUIRE(result.has_value());
    REQUIRE(*result == 999);
}

TEST_CASE("fallback_policy - CALLABLE with manual std::any returning string", "[fallback_policy][callable]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_callable([]() -> std::any { return std::string("Manual any"); });
    const std::optional<std::string> result = policy.get_value<std::string>();

    REQUIRE(result.has_value());
    REQUIRE(*result == "Manual any");
}

TEST_CASE("fallback_policy - get_value_or returns stored value when type matches", "[fallback_policy][get_value_or]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);
    int result = policy.get_value_or(999);

    REQUIRE(result == 42);
}

TEST_CASE("fallback_policy - get_value_or returns default value when type mismatches", "[fallback_policy][get_value_or]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);
    std::string result = policy.get_value_or(std::string("default"));

    REQUIRE(result == "default");
}

TEST_CASE("fallback_policy - get_value_or returns default-constructed for default policy", "[fallback_policy][get_value_or]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();
    std::string result = policy.get_value_or(std::string("fallback"));

    REQUIRE(result == "");
}

TEST_CASE("fallback_policy - get_value is noexcept with throwing callable", "[fallback_policy][noexcept]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([]() -> int 
    {
        throw std::runtime_error("Simulated error");
        return 42;
    });

    const std::optional<int> result = policy.get_value<int>();

    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("fallback_policy - get_value_or is noexcept with throwing callable", "[fallback_policy][noexcept]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([]() -> int 
    {
        throw std::runtime_error("Simulated error");
        return 42;
    });

    int result = policy.get_value_or(999);

    REQUIRE(result == 999);
}

TEST_CASE("fallback_policy - can_cast_to returns true for matching type", "[fallback_policy][type_checking]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);

    REQUIRE(policy.can_cast_to<int>());
}

TEST_CASE("fallback_policy - can_cast_to returns false for non-matching type", "[fallback_policy][type_checking]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);

    REQUIRE_FALSE(policy.can_cast_to<std::string>());
}

TEST_CASE("fallback_policy - can_cast_to returns true for default-constructible types with DEFAULT policy",
    "[fallback_policy][type_checking]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();

    REQUIRE(policy.can_cast_to<int>());
    REQUIRE(policy.can_cast_to<std::string>());
    REQUIRE(policy.can_cast_to<std::vector<int>>());
}

TEST_CASE("fallback_policy - stored_type returns correct type_info for specific value", "[fallback_policy][type_checking]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);

    REQUIRE(policy.stored_type() == typeid(int));
}

TEST_CASE("fallback_policy - stored_type returns void for default policy", "[fallback_policy][type_checking]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();

    REQUIRE(policy.stored_type() == typeid(void));
}

TEST_CASE("fallback_policy - make_default_fallback creates DEFAULT policy", "[fallback_policy][helpers]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();

    REQUIRE(policy.get_type() == shield::fallback_type::DEFAULT);

    const std::optional<int> result = policy.get_value<int>();
    REQUIRE(result.has_value());
    REQUIRE(*result == 0);
}

TEST_CASE("fallback_policy - make_value_fallback creates SPECIFIC_VALUE policy", "[fallback_policy][helpers]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);

    REQUIRE(policy.get_type() == shield::fallback_type::SPECIFIC_VALUE);

    const std::optional<int> result = policy.get_value<int>();
    REQUIRE(result.has_value());
    REQUIRE(*result == 42);
}

TEST_CASE("fallback_policy - make_callable_fallback creates CALLABLE policy", "[fallback_policy][helpers]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([]() { return std::string("Helper result"); });

    REQUIRE(policy.get_type() == shield::fallback_type::CALLABLE);

    const std::optional<std::string> result = policy.get_value<std::string>();
    REQUIRE(result.has_value());
    REQUIRE(*result == "Helper result");
}

TEST_CASE("fallback_policy - type erasure stores different types in same container", "[fallback_policy][type_erasure]")
{
    std::vector<shield::fallback_policy> policies;

    policies.push_back(shield::fallback_policy::with_value(42));
    policies.push_back(shield::fallback_policy::with_value(std::string("error")));
    policies.push_back(shield::fallback_policy::with_value(3.14159));

    REQUIRE(policies.size() == 3);

    const std::optional<int> int_val = policies[0].get_value<int>();
    const std::optional<std::string> str_val = policies[1].get_value<std::string>();
    const std::optional<double> dbl_val = policies[2].get_value<double>();

    REQUIRE(int_val.has_value());
    REQUIRE(*int_val == 42);

    REQUIRE(str_val.has_value());
    REQUIRE(*str_val == "error");

    REQUIRE(dbl_val.has_value());
    REQUIRE(*dbl_val == 3.14159);
}

TEST_CASE("fallback_policy - move semantics with large vector", "[fallback_policy][move]")
{
    std::vector<int> large_vec(1000, 42);
    shield::fallback_policy policy = shield::fallback_policy::with_value(std::move(large_vec));

    std::optional<std::vector<int>> result = policy.get_value<std::vector<int>>();
    REQUIRE(result.has_value());
    REQUIRE(result->size() == 1000);
    REQUIRE(result->at(0) == 42);
}

TEST_CASE("fallback_policy - move policy into another policy", "[fallback_policy][move]")
{
    shield::fallback_policy policy1 = shield::fallback_policy::with_value(42);
    shield::fallback_policy policy2 = std::move(policy1);

    std::optional<int> result = policy2.get_value<int>();
    REQUIRE(result.has_value());
    REQUIRE(*result == 42);
}

TEST_CASE("fallback_policy - execute returns std::any", "[fallback_policy][execute]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(42);
    std::any result = policy.execute();

    REQUIRE(result.has_value());
    REQUIRE(result.type() == typeid(int));
    REQUIRE(std::any_cast<int>(result) == 42);
}

TEST_CASE("fallback_policy - operator() returns std::any", "[fallback_policy][execute]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(std::string("test"));
    std::any result = policy();

    REQUIRE(result.has_value());
    REQUIRE(result.type() == typeid(std::string));
    REQUIRE(std::any_cast<std::string>(result) == "test");
}

TEST_CASE("fallback_policy - execute returns empty any for DEFAULT policy", "[fallback_policy][execute]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_default();
    std::any result = policy.execute();

    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("fallback_policy - throws on null callable", "[fallback_policy][validation]")
{
    REQUIRE_THROWS_AS(shield::fallback_policy::with_callable(nullptr), std::invalid_argument);
}

TEST_CASE("fallback_policy - to_string converts DEFAULT to string", "[fallback_policy][utility]")
{
    REQUIRE(shield::to_string(shield::fallback_type::DEFAULT) == "DEFAULT");
}

TEST_CASE("fallback_policy - to_string converts SPECIFIC_VALUE to string", "[fallback_policy][utility]")
{
    REQUIRE(shield::to_string(shield::fallback_type::SPECIFIC_VALUE) == "SPECIFIC_VALUE");
}

TEST_CASE("fallback_policy - to_string converts CALLABLE to string", "[fallback_policy][utility]")
{
    REQUIRE(shield::to_string(shield::fallback_type::CALLABLE) == "CALLABLE");
}

TEST_CASE("fallback_policy - circuit breaker integration uses fallback when circuit is open", "[fallback_policy][integration]")
{
    shield::fallback_policy circuit_fallback = shield::fallback_policy::with_typed_callable([]()
    {
        return ServiceResponse{ 503, "Circuit breaker is OPEN - using fallback", false };
    });

    bool circuit_is_open = true;

    ServiceResponse response;
    if (circuit_is_open)
    {
        std::optional<ServiceResponse> fallback_response = circuit_fallback.get_value<ServiceResponse>();
        if (fallback_response)
        {
            response = *fallback_response;
        }
    }

    REQUIRE(response.status_code == 503);
    REQUIRE(response.message == "Circuit breaker is OPEN - using fallback");
    REQUIRE_FALSE(response.success);
}

TEST_CASE("fallback_policy - fallback chain with multiple policies", "[fallback_policy][complex]")
{
    shield::fallback_policy primary = shield::fallback_policy::with_typed_callable([]() -> int
    {
        throw std::runtime_error("Primary failed");
    });

    shield::fallback_policy secondary = shield::fallback_policy::with_value(100);
    shield::fallback_policy tertiary = shield::fallback_policy::with_value(999);

    // Try primary
    std::optional<int> result = primary.get_value<int>();
    if (!result)
    {
        // Try secondary
        result = secondary.get_value<int>();
    }
    if (!result)
    {
        // Try tertiary
        result = tertiary.get_value<int>();
    }

    REQUIRE(result.has_value());
    REQUIRE(*result == 100);
}

TEST_CASE("fallback_policy - dynamic type selection based on runtime condition", "[fallback_policy][complex]")
{
    std::function<shield::fallback_policy(bool)> get_policy = [](bool use_string) -> shield::fallback_policy
    {
        if (use_string)
        {
            return shield::fallback_policy::with_value(std::string("text"));
        }
        else
        {
            return shield::fallback_policy::with_value(42);
        }
    };

    shield::fallback_policy string_policy = get_policy(true);
    shield::fallback_policy int_policy = get_policy(false);

    std::optional<std::string> str_result = string_policy.get_value<std::string>();
    std::optional<int> int_result = int_policy.get_value<int>();

    REQUIRE(str_result.has_value());
    REQUIRE(*str_result == "text");

    REQUIRE(int_result.has_value());
    REQUIRE(*int_result == 42);
}

TEST_CASE("fallback_policy - deterministic behavior across multiple creations", "[fallback_policy]")
{
    for (int i = 0; i < 20; ++i)
    {
        shield::fallback_policy policy = shield::fallback_policy::with_value(42);

        REQUIRE(policy.get_type() == shield::fallback_type::SPECIFIC_VALUE);
        REQUIRE(policy.has_specific_value());

        std::optional<int> result = policy.get_value<int>();
        REQUIRE(result.has_value());
        REQUIRE(*result == 42);
    }
}

TEST_CASE("fallback_policy - multiple retrievals return consistent results", "[fallback_policy]")
{
    shield::fallback_policy policy = shield::fallback_policy::with_value(std::string("consistent"));

    for (int i = 0; i < 10; ++i)
    {
        std::optional<std::string> result = policy.get_value<std::string>();
        REQUIRE(result.has_value());
        REQUIRE(*result == "consistent");
    }
}

TEST_CASE("fallback_policy - callable invoked each time", "[fallback_policy][callable]")
{
    int counter = 0;
    shield::fallback_policy policy = shield::fallback_policy::with_typed_callable([&counter]() { return ++counter; });

    std::optional<int> result1 = policy.get_value<int>();
    std::optional<int> result2 = policy.get_value<int>();
    std::optional<int> result3 = policy.get_value<int>();

    REQUIRE(result1.has_value());
    REQUIRE(*result1 == 1);

    REQUIRE(result2.has_value());
    REQUIRE(*result2 == 2);

    REQUIRE(result3.has_value());
    REQUIRE(*result3 == 3);
}