// ============================================================================
// shield_cpp - A C++ Resilience Library
// ============================================================================
// 
// #ifndef SHIELD_CPP_HPP
// #define SHIELD_CPP_HPP
// 
// #include <iostream>
// #include <chrono>
// #include <thread>
// #include <functional>
// #include <atomic>
// #include <mutex>
// #include <future>
// #include <queue>
// #include <memory>
// #include <boost/asio.hpp>
// #include <folly/executors/ThreadedExecutor.h>
// #include <folly/futures/Future.h>
// #include <prometheus/counter.h>
// #include <prometheus/gauge.h>
// #include <prometheus/registry.h>
// 
// namespace shield_cpp
// {
// 
// ============================================================================
// 1. RETRY PATTERN
// ============================================================================
// template<typename Func>
// auto retry(Func&& func, int max_attempts = 3, 
//            std::chrono::milliseconds delay = std::chrono::milliseconds(100))
// {
//     using return_type = decltype(func());
//     
//     for (int attempt = 1; attempt <= max_attempts; ++attempt)
//     {
//         try
//         {
//             return func();
//         }
//         catch (const std::exception& e)
//         {
//             if (attempt == max_attempts)
//             {
//                 throw;
//             }
//             std::cout << "Attempt " << attempt << " failed: " << e.what() 
//                       << ". Retrying...\n";
//             std::this_thread::sleep_for(delay);
//             delay *= 2; // Exponential backoff
//         }
//     }
//     throw std::runtime_error("Retry failed");
// }
// 
// ============================================================================
// 2. CIRCUIT BREAKER PATTERN
// ============================================================================
// class circuit_breaker
// {
// public:
//     enum class state
//     {
//         closed,
//         open,
//         half_open
//     };
//     
//     circuit_breaker(int failure_threshold = 5, 
//                    std::chrono::seconds timeout = std::chrono::seconds(60))
//         : failure_threshold_(failure_threshold)
//         , timeout_(timeout)
//         , failure_count_(0)
//         , state_(state::closed)
//     {
//     }
//     
//     template<typename Func>
//     auto execute(Func&& func)
//     {
//         if (state_ == state::open)
//         {
//             auto now = std::chrono::steady_clock::now();
//             if (now - last_failure_time_ > timeout_)
//             {
//                 std::cout << "Circuit transitioning to HALF_OPEN\n";
//                 state_ = state::half_open;
//             }
//             else
//             {
//                 throw std::runtime_error("Circuit breaker is OPEN");
//             }
//         }
//         
//         try
//         {
//             auto result = func();
//             on_success();
//             return result;
//         }
//         catch (...)
//         {
//             on_failure();
//             throw;
//         }
//     }
//     
//     state get_state() const
//     {
//         return state_;
//     }
//     
//     int get_failure_count() const
//     {
//         return failure_count_;
//     }
//     
// private:
//     void on_success()
//     {
//         std::lock_guard<std::mutex> lock(mutex_);
//         failure_count_ = 0;
//         if (state_ == state::half_open)
//         {
//             std::cout << "Circuit transitioning to CLOSED\n";
//             state_ = state::closed;
//         }
//     }
//     
//     void on_failure()
//     {
//         std::lock_guard<std::mutex> lock(mutex_);
//         failure_count_++;
//         last_failure_time_ = std::chrono::steady_clock::now();
//         
//         if (failure_count_ >= failure_threshold_ && state_ != state::open)
//         {
//             std::cout << "Circuit transitioning to OPEN\n";
//             state_ = state::open;
//         }
//     }
//     
//     int failure_threshold_;
//     std::chrono::seconds timeout_;
//     std::atomic<int> failure_count_;
//     std::atomic<state> state_;
//     std::chrono::steady_clock::time_point last_failure_time_;
//     std::mutex mutex_;
// };
// 
// ============================================================================
// 3. TIMEOUT PATTERN (using std::async)
// ============================================================================
// template<typename Func>
// auto with_timeout(Func&& func, std::chrono::milliseconds timeout)
// {
//     using return_type = decltype(func());
//     
//     auto future = std::async(std::launch::async, std::forward<Func>(func));
//     
//     if (future.wait_for(timeout) == std::future_status::timeout)
//     {
//         throw std::runtime_error("Operation timed out");
//     }
//     
//     return future.get();
// }
// 
// Alternative using Boost.Asio
// class timeout_executor
// {
// public:
//     timeout_executor()// : work_(io_context_)
//     {
//         thread_ = std::thread([this]()
//         {
//             io_context_.run();
//         });
//     }
//     
//     ~timeout_executor()
//     {
//         io_context_.stop();
//         if (thread_.joinable())
//         {
//             thread_.join();
//         }
//     }
//     
//     template<typename Func>
//     auto execute_with_timeout(Func&& func, std::chrono::milliseconds timeout)
//     {
//         using return_type = decltype(func());
//         auto promise = std::make_shared<std::promise<return_type>>();
//         auto future = promise->get_future();
//         
//         boost::asio::steady_timer timer(io_context_, timeout);
//         std::atomic<bool> completed{false};
//         
//         // Execute function in separate thread
//         std::thread([func, promise, &completed]()
//         {
//             try
//             {
//                 if constexpr (std::is_void_v<return_type>)
//                 {
//                     func();
//                     promise->set_value();
//                 }
//                 else
//                 {
//                     promise->set_value(func());
//                 }
//                 completed = true;
//             }
//             catch (...)
//             {
//                 promise->set_exception(std::current_exception());
//                 completed = true;
//             }
//         }).detach();
//         
//         // Setup timeout
//         timer.async_wait([promise, &completed](const boost::system::error_code& ec)
//         {
//             if (!ec && !completed)
//             {
//                 promise->set_exception(
//                     std::make_exception_ptr(std::runtime_error("Timeout")));
//             }
//         });
//         
//         return future.get();
//     }
//     
// private:
//     boost::asio::io_context io_context_;
//     //boost::asio::io_context::work work_;
//     std::thread thread_;
// };
// 
// ============================================================================
// 4. BULKHEAD PATTERN (using Folly)
// ============================================================================
// class bulkhead
// {
// public:
//     explicit bulkhead(size_t max_concurrent = 10)
//         : max_concurrent_(max_concurrent)
//         , current_count_(0)
//         , executor_(std::make_unique<folly::ThreadedExecutor>())
//     {
//     }
//     
//     template<typename Func>
//     folly::Future<typename std::result_of<Func()>::type> execute(Func&& func)
//     {
//         if (current_count_ >= max_concurrent_)
//         {
//             return folly::makeFuture<typename std::result_of<Func()>::type>(
//                 std::runtime_error("Bulkhead capacity exceeded"));
//         }
//         
//         current_count_++;
//         
//         return folly::via(executor_.get())
//             .thenValue([this, f = std::forward<Func>(func)](auto&&) mutable
//             {
//                 try
//                 {
//                     auto result = f();
//                     current_count_--;
//                     return result;
//                 }
//                 catch (...)
//                 {
//                     current_count_--;
//                     throw;
//                 }
//             });
//     }
//     
//     size_t get_current_count() const
//     {
//         return current_count_.load();
//     }
//     
//     size_t get_max_concurrent() const
//     {
//         return max_concurrent_;
//     }
//     
// private:
//     size_t max_concurrent_;
//     std::atomic<size_t> current_count_;
//     std::unique_ptr<folly::ThreadedExecutor> executor_;
// };
// 
// ============================================================================
// 5. FALLBACK PATTERN
// ============================================================================
// template<typename Func, typename FallbackFunc>
// auto with_fallback(Func&& primary, FallbackFunc&& fallback)
// {
//     try
//     {
//         return primary();
//     }
//     catch (const std::exception& e)
//     {
//         std::cout << "Primary failed: " << e.what() 
//                   << ". Using fallback.\n";
//         return fallback();
//     }
// }
// 
// ============================================================================
// COMBINED PATTERN WITH PROMETHEUS METRICS
// ============================================================================
// class resilient_service
// {
// public:
//     resilient_service(std::shared_ptr<prometheus::Registry> registry)
//         : circuit_breaker_(5, std::chrono::seconds(30))
//         , bulkhead_(10)
//         , request_counter_(prometheus::BuildCounter()
//             .Name("requests_total")
//             .Help("Total number of requests")
//             .Register(*registry)
//             .Add({{"service", "resilient"}}))
//         , failure_counter_(prometheus::BuildCounter()
//             .Name("failures_total")
//             .Help("Total number of failures")
//             .Register(*registry)
//             .Add({{"service", "resilient"}}))
//         , active_requests_(prometheus::BuildGauge()
//             .Name("active_requests")
//             .Help("Number of active requests")
//             .Register(*registry)
//             .Add({{"service", "resilient"}}))
//     {
//     }
//     
//     template<typename Func, typename FallbackFunc>
//     auto execute_resilient(Func&& func, FallbackFunc&& fallback)
//     {
//         request_counter_.Increment();
//         active_requests_.Increment();
//         
//         try
//         {
//             auto result = circuit_breaker_.execute([&]()
//             {
//                 return retry([&]()
//                 {
//                     return with_timeout(std::forward<Func>(func), 
//                                        std::chrono::seconds(5));
//                 }, 3);
//             });
//             active_requests_.Decrement();
//             return result;
//         }
//         catch (const std::exception& e)
//         {
//             failure_counter_.Increment();
//             active_requests_.Decrement();
//             return with_fallback(
//                 [&]() -> decltype(func())
//                 {
//                     throw;
//                 },
//                 std::forward<FallbackFunc>(fallback)
//             );
//         }
//     }
//     
// private:
//     circuit_breaker circuit_breaker_;
//     bulkhead bulkhead_;
//     prometheus::Counter& request_counter_;
//     prometheus::Counter& failure_counter_;
//     prometheus::Gauge& active_requests_;
// };
// 
// } // namespace shield_cpp
// 
// #endif // SHIELD_CPP_HPP