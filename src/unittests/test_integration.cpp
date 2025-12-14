// #include <catch2/catch_test_macros.hpp>
// #include <shield/all.hpp>
// 
// using namespace shield;
// 
// TEST_CASE("Integration - retry with timeout", "[integration]")
// {
//     int attempt = 0;
//     
//     auto result = retry([&attempt]()
//     {
//         attempt++;
//         return with_timeout([]()
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));
//             return 42;
//         }, std::chrono::milliseconds(100));
//     }, 3);
//     
//     REQUIRE(result == 42);
//     REQUIRE(attempt == 1);
// }
// 
// TEST_CASE("Integration - retry with timeout exceeding", "[integration]")
// {
//     int attempt = 0;
//     
//     REQUIRE_THROWS_AS(
//         retry([&attempt]()
//         {
//             attempt++;
//             return with_timeout([]()
//             {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(200));
//                 return 42;
//             }, std::chrono::milliseconds(50));
//         }, 2),
//         std::runtime_error
//     );
//     
//     REQUIRE(attempt == 2);
// }
// 
// TEST_CASE("Integration - circuit breaker with retry", "[integration]")
// {
//     circuit_breaker cb(3, std::chrono::seconds(1));
//     int total_attempts = 0;
//     
//     // First call succeeds after retry
//     auto result1 = cb.execute([&total_attempts]()
//     {
//         return retry([&total_attempts]()
//         {
//             total_attempts++;
//             if (total_attempts < 2)
//             {
//                 throw std::runtime_error("Temporary failure");
//             }
//             return 100;
//         }, 3);
//     });
//     
//     REQUIRE(result1 == 100);
//     REQUIRE(cb.get_state() == circuit_breaker::state::closed);
// }
// 
// TEST_CASE("Integration - retry with fallback", "[integration]")
// {
//     int primary_attempts = 0;
//     
//     auto result = with_fallback(
//         [&primary_attempts]()
//         {
//             return retry([&primary_attempts]()
//             {
//                 primary_attempts++;
//                 throw std::runtime_error("Always fails");
//                 return 1;
//             }, 2);
//         },
//         []() { return 999; }
//     );
//     
//     REQUIRE(result == 999);
//     REQUIRE(primary_attempts == 2);
// }
// 
// TEST_CASE("Integration - circuit breaker with fallback", "[integration]")
// {
//     circuit_breaker cb(2, std::chrono::seconds(10));
//     
//     // Open the circuit
//     for (int i = 0; i < 2; i++)
//     {
//         try
//         {
//             cb.execute([]() -> int
//             {
//                 throw std::runtime_error("Service down");
//             });
//         }
//         catch (...) {}
//     }
//     
//     REQUIRE(cb.get_state() == circuit_breaker::state::open);
//     
//     // Use fallback when circuit is open
//     auto result = with_fallback(
//         [&cb]()
//         {
//             return cb.execute([]() { return 42; });
//         },
//         []() { return -1; }
//     );
//     
//     REQUIRE(result == -1);
// }
// 
// TEST_CASE("Integration - all patterns combined", "[integration]")
// {
//     circuit_breaker cb(3, std::chrono::milliseconds(100));
//     int call_count = 0;
//     
//     auto result = with_fallback(
//         [&cb, &call_count]()
//         {
//             return cb.execute([&call_count]()
//             {
//                 return retry([&call_count]()
//                 {
//                     call_count++;
//                     return with_timeout([]()
//                     {
//                         std::this_thread::sleep_for(std::chrono::milliseconds(10));
//                         return 42;
//                     }, std::chrono::milliseconds(100));
//                 }, 2);
//             });
//         },
//         []() { return 0; }
//     );
//     
//     REQUIRE(result == 42);
//     REQUIRE(call_count == 1);
//     REQUIRE(cb.get_state() == circuit_breaker::state::closed);
// }
// 
// TEST_CASE("Integration - cascading failures with fallback", "[integration]")
// {
//     circuit_breaker cb(2, std::chrono::seconds(1));
//     
//     // Fail enough times to open circuit
//     for (int i = 0; i < 2; i++)
//     {
//         try
//         {
//             cb.execute([i]()
//             {
//                 return retry([]() -> int
//                 {
//                     throw std::runtime_error("Service failure");
//                 }, 2);
//             });
//         }
//         catch (...) {}
//     }
//     
//     REQUIRE(cb.get_state() == circuit_breaker::state::open);
//     
//     // Final call uses fallback due to open circuit
//     auto result = with_fallback(
//         [&cb]() { return cb.execute([]() { return 1; }); },
//         []() { return 100; }
//     );
//     
//     REQUIRE(result == 100);
// }
// 
// TEST_CASE("Integration - timeout with fallback recovery", "[integration]")
// {
//     bool used_fallback = false;
//     
//     auto result = with_fallback(
//         []()
//         {
//             return with_timeout([]()
//             {
//                 std::this_thread::sleep_for(std::chrono::milliseconds(200));
//                 return 42;
//             }, std::chrono::milliseconds(50));
//         },
//         [&used_fallback]()
//         {
//             used_fallback = true;
//             return 999;
//         }
//     );
//     
//     REQUIRE(result == 999);
//     REQUIRE(used_fallback);
// }
// 
// TEST_CASE("Integration - resilient service with prometheus", "[integration]")
// {
//     auto registry = std::make_shared<prometheus::Registry>();
//     resilient_service service(registry);
//     
//     auto result = service.execute_resilient(
//         []()
//         {
//             std::this_thread::sleep_for(std::chrono::milliseconds(10));
//             return std::string("primary success");
//         },
//         []()
//         {
//             return std::string("fallback success");
//         }
//     );
//     
//     REQUIRE(result == "primary success");
// }
// 
// TEST_CASE("Integration - resilient service falls back", "[integration]")
// {
//     auto registry = std::make_shared<prometheus::Registry>();
//     resilient_service service(registry);
//     
//     auto result = service.execute_resilient(
//         []() -> std::string
//         {
//             throw std::runtime_error("Primary failed");
//         },
//         []()
//         {
//             return std::string("fallback used");
//         }
//     );
//     
//     REQUIRE(result == "fallback used");
// }
// 
// TEST_CASE("Integration - complex real-world scenario", "[integration]")
// {
//     circuit_breaker cb(3, std::chrono::milliseconds(200));
//     int attempt_count = 0;
//     bool used_fallback = false;
//     
//     // Simulate API call with full resilience
//     auto make_api_call = [&cb, &attempt_count, &used_fallback]()
//     {
//         return with_fallback(
//             [&cb, &attempt_count]()
//             {
//                 return cb.execute([&attempt_count]()
//                 {
//                     return retry([&attempt_count]()
//                     {
//                         attempt_count++;
//                         return with_timeout([]()
//                         {
//                             // Simulate API call
//                             std::this_thread::sleep_for(std::chrono::milliseconds(5));
//                             return std::string("API response");
//                         }, std::chrono::milliseconds(50));
//                     }, 3);
//                 });
//             },
//             [&used_fallback]()
//             {
//                 used_fallback = true;
//                 return std::string("Cached response");
//             }
//         );
//     };
//     
//     auto result = make_api_call();
//     
//     REQUIRE(result == "API response");
//     REQUIRE_FALSE(used_fallback);
//     REQUIRE(attempt_count > 0);
// }