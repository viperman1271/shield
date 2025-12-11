#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_exception.hpp>
#include "shield_cpp.hpp"
#include <vector>

using namespace shield_cpp;

TEST_CASE("Bulkhead - executes single task", "[bulkhead]")
{
    bulkhead bh(5);
    
    auto future = bh.execute([]()
    {
        return 42;
    });
    
    REQUIRE(future.get() == 42);
}

TEST_CASE("Bulkhead - respects max concurrent limit", "[bulkhead]")
{
    bulkhead bh(2);
    
    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};
    
    std::vector<folly::Future<int>> futures;
    
    for (int i = 0; i < 5; i++)
    {
        auto future = bh.execute([&concurrent_count, &max_concurrent]()
        {
            int current = ++concurrent_count;
            int expected = max_concurrent.load();
            while (current > expected && 
                   !max_concurrent.compare_exchange_weak(expected, current)) {}
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            concurrent_count--;
            return current;
        });
        
        futures.push_back(std::move(future));
    }
    
    // Wait for all to complete
    for (auto& f : futures)
    {
        try
        {
            f.get();
        }
        catch (...)
        {
            // Some may fail due to bulkhead limit
        }
    }
    
    // Max concurrent should never exceed bulkhead limit
    REQUIRE(max_concurrent.load() <= 2);
}

TEST_CASE("Bulkhead - rejects when capacity exceeded", "[bulkhead]")
{
    bulkhead bh(1);
    
    // First task blocks
    auto future1 = bh.execute([]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return 1;
    });
    
    // Give first task time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Second task should be rejected
    auto future2 = bh.execute([]()
    {
        return 2;
    });
    
    // First should succeed
    REQUIRE(future1.get() == 1);
    
    // Second should fail
    REQUIRE_THROWS_AS(future2.get(), std::runtime_error);
}

TEST_CASE("Bulkhead - tracks current count correctly", "[bulkhead]")
{
    bulkhead bh(3);
    
    REQUIRE(bh.get_current_count() == 0);
    REQUIRE(bh.get_max_concurrent() == 3);
    
    std::vector<folly::Future<int>> futures;
    
    // Start 2 long-running tasks
    for (int i = 0; i < 2; i++)
    {
        futures.push_back(bh.execute([i]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            return i;
        }));
    }
    
    // Current count should reflect running tasks
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    REQUIRE(bh.get_current_count() <= 2);
    
    // Wait for completion
    for (auto& f : futures)
    {
        f.get();
    }
    
    // Give time for cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(bh.get_current_count() == 0);
}

TEST_CASE("Bulkhead - handles exceptions in tasks", "[bulkhead]")
{
    bulkhead bh(5);
    
    auto future = bh.execute([]() -> int
    {
        throw std::runtime_error("Task failed");
    });
    
    REQUIRE_THROWS_AS(future.get(), std::runtime_error);
    
    // Count should be decremented even after exception
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(bh.get_current_count() == 0);
}

TEST_CASE("Bulkhead - handles void return type", "[bulkhead]")
{
    bulkhead bh(5);
    bool executed = false;
    
    auto future = bh.execute([&executed]()
    {
        executed = true;
    });
    
    future.get();
    REQUIRE(executed);
}

TEST_CASE("Bulkhead - allows sequential execution within limit", "[bulkhead]")
{
    bulkhead bh(2);
    
    for (int i = 0; i < 10; i++)
    {
        auto future = bh.execute([i]()
        {
            return i * 2;
        });
        
        REQUIRE(future.get() == i * 2);
    }
}

TEST_CASE("Bulkhead - stress test with many tasks", "[bulkhead]")
{
    bulkhead bh(5);
    
    std::vector<folly::Future<int>> futures;
    std::atomic<int> successful{0};
    
    for (int i = 0; i < 20; i++)
    {
        auto future = bh.execute([i, &successful]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            successful++;
            return i;
        });
        
        futures.push_back(std::move(future));
    }
    
    // Some will succeed, some will be rejected
    int completed = 0;
    for (auto& f : futures)
    {
        try
        {
            f.get();
            completed++;
        }
        catch (const std::runtime_error&)
        {
            // Expected for tasks exceeding capacity
        }
    }
    
    REQUIRE(successful.load() > 0);
    REQUIRE(completed == successful.load());
}