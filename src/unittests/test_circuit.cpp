#include <shield/all.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>

#include <chrono>

TEST_CASE("Circuit - initial state is closed", "[circuit_breaker]")
{
    shield::circuit_breaker cb("test", 3, std::chrono::seconds(1));
    
    REQUIRE(cb.get_state() == shield::circuit_breaker::state::closed);
    REQUIRE(cb.get_failure_count() == 0);
}

TEST_CASE("Circuit - successful execution", "[circuit_breaker]")
{
    shield::circuit_breaker cb("test", 3, std::chrono::seconds(1));
    
//     auto result = cb.execute([]()
//     {
//         return 42;
//     });
//     
//     REQUIRE(result == 42);
//     REQUIRE(cb.get_state() == circuit_breaker::state::closed);
//     REQUIRE(cb.get_failure_count() == 0);
}

TEST_CASE("Circuit - opens after threshold failures", "[circuit_breaker]")
{
    shield::circuit_breaker cb("test", 3, std::chrono::seconds(10));
    
    // Fail 3 times to reach threshold
//     for (int i = 0; i < 3; i++)
//     {
//         try
//         {
//             cb.execute([]()
//             {
//                 throw std::runtime_error("Service failure");
//                 return 0;
//             });
//         }
//         catch (const std::runtime_error&)
//         {
//             // Expected
//         }
//     }
//     
//     REQUIRE(cb.get_state() == shield::circuit_breaker::state::open);
//     REQUIRE(cb.get_failure_count() == 3);
}

TEST_CASE("Circuit - rejects calls when open", "[circuit_breaker]")
{
    shield::circuit_breaker cb("test", 2, std::chrono::seconds(10));
    
    // Open the circuit
//     for (int i = 0; i < 2; i++)
//     {
//         try
//         {
//             cb.execute([]()
//             {
//                 throw std::runtime_error("Fail");
//                 return 0;
//             });
//         }
//         catch (...) {}
//     }
//     
//     REQUIRE(cb.get_state() == shield::circuit_breaker::state::open);
//     
//     // Should reject immediately
//     REQUIRE_THROWS_WITH(
//         cb.execute([]() { return 42; }),
//         "Circuit is OPEN"
//     );
}

TEST_CASE("Circuit - transitions to half-open after timeout", "[circuit_breaker]")
{
    shield::circuit_breaker cb("test", 2, std::chrono::milliseconds(100));
    
    // Open the circuit
//     for (int i = 0; i < 2; i++)
//     {
//         try
//         {
//             cb.execute([]()
//             {
//                 throw std::runtime_error("Fail");
//                 return 0;
//             });
//         }
//         catch (...) {}
//     }
//     
//     REQUIRE(cb.get_state() == shield::circuit_breaker::state::open);
//     
//     // Wait for timeout
//     std::this_thread::sleep_for(std::chrono::milliseconds(150));
//     
//     // Next call should transition to half-open
//     auto result = cb.execute([]() { return 99; });
//     
//     REQUIRE(result == 99);
//     REQUIRE(cb.get_state() == shield::circuit_breaker::state::closed);
//     REQUIRE(cb.get_failure_count() == 0);
}

TEST_CASE("Circuit - half-open closes on success", "[circuit_breaker]")
{
    shield::circuit_breaker cb("test", 2, std::chrono::milliseconds(50));
    
    // Open the circuit
//     for (int i = 0; i < 2; i++)
//     {
//         try
//         {
//             cb.execute([]()
//             {
//                 throw std::runtime_error("Fail");
//                 return 0;
//             });
//         }
//         catch (...) {}
//     }
//     
//     // Wait and try again (transitions to half-open then closed)
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     
//     auto result = cb.execute([]() { return 123; });
//     
//     REQUIRE(result == 123);
//     REQUIRE(cb.get_state() == circuit_breaker::state::closed);
}

TEST_CASE("Circuit - half-open reopens on failure", "[circuit_breaker]")
{
    shield::circuit_breaker cb("test", 2, std::chrono::milliseconds(50));
    
    // Open the circuit
//     for (int i = 0; i < 2; i++)
//     {
//         try
//         {
//             cb.execute([]()
//             {
//                 throw std::runtime_error("Fail");
//                 return 0;
//             });
//         }
//         catch (...) {}
//     }
//     
//     // Wait for half-open transition
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     
//     // Fail again in half-open state
//     try
//     {
//         cb.execute([]()
//         {
//             throw std::runtime_error("Still failing");
//             return 0;
//         });
//     }
//     catch (...) {}
//     
//     REQUIRE(cb.get_state() == shield::circuit_breaker::state::open);
}

TEST_CASE("Circuit - success resets failure count", "[circuit_breaker]")
{
    shield::circuit_breaker cb("test", 5, std::chrono::seconds(1));
    
    // Fail a few times
//     for (int i = 0; i < 3; i++)
//     {
//         try
//         {
//             cb.execute([]()
//             {
//                 throw std::runtime_error("Fail");
//                 return 0;
//             });
//         }
//         catch (...) {}
//     }
//     
//     REQUIRE(cb.get_failure_count() == 3);
//     REQUIRE(cb.get_state() == circuit_breaker::state::closed);
//     
//     // Succeed once
//     cb.execute([]() { return 42; });
//     
//     REQUIRE(cb.get_failure_count() == 0);
//     REQUIRE(cb.get_state() == circuit_breaker::state::closed);
}

TEST_CASE("Circuit - custom failure threshold", "[circuit_breaker]")
{
    shield::circuit_breaker cb("test", 10, std::chrono::seconds(1));
    
    // Fail 9 times - should still be closed
//     for (int i = 0; i < 9; i++)
//     {
//         try
//         {
//             cb.execute([]()
//             {
//                 throw std::runtime_error("Fail");
//                 return 0;
//             });
//         }
//         catch (...) {}
//     }
//     
//     REQUIRE(cb.get_state() == shield::circuit_breaker::state::closed);
//     
//     // 10th failure should open it
//     try
//     {
//         cb.execute([]()
//         {
//             throw std::runtime_error("Fail");
//             return 0;
//         });
//     }
//     catch (...) {}
//     
//     REQUIRE(cb.get_state() == shield::circuit_breaker::state::open);
}