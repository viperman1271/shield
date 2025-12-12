// #include <catch2/catch_test_macros.hpp>
// #include <catch2/matchers/catch_matchers_exception.hpp>
// #include <shield/all.hpp>
// 
// using namespace shield;
// 
// TEST_CASE("Timeout - completes within timeout", "[timeout]")
// {
//     auto result = with_timeout([]()
//     {
//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//         return 42;
//     }, std::chrono::milliseconds(200));
//     
//     REQUIRE(result == 42);
// }
// 
// TEST_CASE("Timeout - throws when exceeding timeout", "[timeout]")
// {
//     REQUIRE_THROWS_WITH(
//         with_timeout([]()
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(500));
//             return 42;
//         }, std::chrono::milliseconds(50)),
//         "Operation timed out"
//     );
// }
// 
// TEST_CASE("Timeout - handles void return type", "[timeout]")
// {
//     bool executed = false;
//     
//     REQUIRE_NOTHROW(
//         with_timeout([&executed]()
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));
//             executed = true;
//         }, std::chrono::milliseconds(100))
//     );
//     
//     REQUIRE(executed);
// }
// 
// TEST_CASE("Timeout - propagates exceptions from function", "[timeout]")
// {
//     REQUIRE_THROWS_AS(
//         with_timeout([]()
//         {
//             throw std::logic_error("Custom error");
//             return 42;
//         }, std::chrono::seconds(1)),
//         std::logic_error
//     );
// }
// 
// TEST_CASE("Timeout - handles different timeout durations", "[timeout]")
// {
//     // Very short timeout
//     REQUIRE_THROWS_AS(
//         with_timeout([]()
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(100));
//             return 1;
//         }, std::chrono::milliseconds(1)),
//         std::runtime_error
//     );
//     
//     // Generous timeout
//     auto result = with_timeout([]()
//     {
//         return 99;
//     }, std::chrono::seconds(10));
//     
//     REQUIRE(result == 99);
// }
// 
// TEST_CASE("Timeout - handles string return type", "[timeout]")
// {
//     auto result = with_timeout([]()
//     {
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//         return std::string("success");
//     }, std::chrono::milliseconds(100));
//     
//     REQUIRE(result == "success");
// }
// 
// TEST_CASE("Timeout - precise timing for boundary conditions", "[timeout]")
// {
//     auto start = std::chrono::steady_clock::now();
//     
//     try
//     {
//         with_timeout([]()
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(150));
//             return 42;
//         }, std::chrono::milliseconds(100));
//         
//         FAIL("Should have timed out");
//     }
//     catch (const std::runtime_error&)
//     {
//         auto end = std::chrono::steady_clock::now();
//         auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
//         
//         // Should timeout close to 100ms, definitely before 150ms
//         REQUIRE(duration.count() >= 90);
//         REQUIRE(duration.count() <= 140);
//     }
// }
// 
// TEST_CASE("Timeout - handles complex return types", "[timeout]")
// {
//     struct result_type
//     {
//         int value;
//         std::string message;
//     };
//     
//     auto result = with_timeout([]()
//     {
//         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//         return result_type{42, "test"};
//     }, std::chrono::milliseconds(100));
//     
//     REQUIRE(result.value == 42);
//     REQUIRE(result.message == "test");
// }
// 
// TEST_CASE("Timeout executor - completes within timeout", "[timeout][executor]")
// {
//     timeout_executor executor;
//     
//     auto result = executor.execute_with_timeout([]()
//     {
//         std::this_thread::sleep_for(std::chrono::milliseconds(50));
//         return 100;
//     }, std::chrono::milliseconds(200));
//     
//     REQUIRE(result == 100);
// }
// 
// TEST_CASE("Timeout executor - throws on timeout", "[timeout][executor]")
// {
//     timeout_executor executor;
//     
//     REQUIRE_THROWS_AS(
//         executor.execute_with_timeout([]()
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(500));
//             return 42;
//         }, std::chrono::milliseconds(50)),
//         std::runtime_error
//     );
// }
// 
// TEST_CASE("Timeout executor - handles void return", "[timeout][executor]")
// {
//     timeout_executor executor;
//     bool executed = false;
//     
//     REQUIRE_NOTHROW(
//         executor.execute_with_timeout([&executed]()
//         {
//             executed = true;
//         }, std::chrono::milliseconds(100))
//     );
//     
//     REQUIRE(executed);
// }