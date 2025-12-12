// #include <catch2/catch_test_macros.hpp>
// #include <shield/all.hpp>
// 
// using namespace shield;
// 
// TEST_CASE("Fallback - uses primary when successful", "[fallback]")
// {
//     auto result = with_fallback(
//         []() { return 42; },
//         []() { return 0; }
//     );
//     
//     REQUIRE(result == 42);
// }
// 
// TEST_CASE("Fallback - uses fallback on primary failure", "[fallback]")
// {
//     auto result = with_fallback(
//         []() -> int { throw std::runtime_error("Primary failed"); },
//         []() { return 99; }
//     );
//     
//     REQUIRE(result == 99);
// }
// 
// TEST_CASE("Fallback - propagates fallback exception", "[fallback]")
// {
//     REQUIRE_THROWS_AS(
//         with_fallback(
//             []() -> int { throw std::runtime_error("Primary failed"); },
//             []() -> int { throw std::logic_error("Fallback also failed"); }
//         ),
//         std::logic_error
//     );
// }
// 
// TEST_CASE("Fallback - handles different return types", "[fallback]")
// {
//     auto string_result = with_fallback(
//         []() -> std::string { throw std::runtime_error("Fail"); },
//         []() { return std::string("fallback"); }
//     );
//     
//     REQUIRE(string_result == "fallback");
//     
//     auto double_result = with_fallback(
//         []() { return 3.14; },
//         []() { return 0.0; }
//     );
//     
//     REQUIRE(double_result == 3.14);
// }
// 
// TEST_CASE("Fallback - handles void return type", "[fallback]")
// {
//     bool primary_called = false;
//     bool fallback_called = false;
//     
//     with_fallback(
//         [&primary_called, &fallback_called]()
//         {
//             primary_called = true;
//             throw std::runtime_error("Fail");
//         },
//         [&fallback_called]()
//         {
//             fallback_called = true;
//         }
//     );
//     
//     REQUIRE(primary_called);
//     REQUIRE(fallback_called);
// }
// 
// TEST_CASE("Fallback - does not call fallback on success", "[fallback]")
// {
//     bool fallback_called = false;
//     
//     auto result = with_fallback(
//         []() { return 123; },
//         [&fallback_called]()
//         {
//             fallback_called = true;
//             return 0;
//         }
//     );
//     
//     REQUIRE(result == 123);
//     REQUIRE_FALSE(fallback_called);
// }
// 
// TEST_CASE("Fallback - handles complex return types", "[fallback]")
// {
//     struct data
//     {
//         int code;
//         std::string message;
//     };
//     
//     auto result = with_fallback(
//         []() -> data { throw std::runtime_error("Primary failed"); },
//         []() { return data{500, "Using cache"}; }
//     );
//     
//     REQUIRE(result.code == 500);
//     REQUIRE(result.message == "Using cache");
// }
// 
// TEST_CASE("Fallback - handles different exception types", "[fallback]")
// {
//     // Runtime error
//     auto result1 = with_fallback(
//         []() -> int { throw std::runtime_error("Runtime error"); },
//         []() { return 1; }
//     );
//     REQUIRE(result1 == 1);
//     
//     // Logic error
//     auto result2 = with_fallback(
//         []() -> int { throw std::logic_error("Logic error"); },
//         []() { return 2; }
//     );
//     REQUIRE(result2 == 2);
//     
//     // Custom exception
//     auto result3 = with_fallback(
//         []() -> int { throw std::invalid_argument("Invalid"); },
//         []() { return 3; }
//     );
//     REQUIRE(result3 == 3);
// }
// 
// TEST_CASE("Fallback - can be nested", "[fallback]")
// {
//     auto result = with_fallback(
//         []() -> int { throw std::runtime_error("Primary failed"); },
//         []()
//         {
//             return with_fallback(
//                 []() -> int { throw std::runtime_error("Secondary failed"); },
//                 []() { return 777; }
//             );
//         }
//     );
//     
//     REQUIRE(result == 777);
// }
// 
// TEST_CASE("Fallback - preserves lambda captures", "[fallback]")
// {
//     int primary_value = 10;
//     int fallback_value = 20;
//     
//     auto result = with_fallback(
//         [primary_value]() -> int { throw std::runtime_error("Fail"); },
//         [fallback_value]() { return fallback_value * 2; }
//     );
//     
//     REQUIRE(result == 40);
// }
// 
// TEST_CASE("Fallback - works with stateful lambdas", "[fallback]")
// {
//     int call_count = 0;
//     
//     auto result = with_fallback(
//         [&call_count]() -> int
//         {
//             call_count++;
//             throw std::runtime_error("Fail");
//         },
//         [&call_count]()
//         {
//             call_count++;
//             return call_count;
//         }
//     );
//     
//     REQUIRE(result == 2);
//     REQUIRE(call_count == 2);
// }