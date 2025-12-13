#pragma once

#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <vector>

namespace shield
{
// ============================================================================
// BACKOFF STRATEGY INTERFACE
// ============================================================================
class backoff_strategy
{
public:
    virtual ~backoff_strategy() = default;
    virtual std::chrono::milliseconds calculate_delay(int attempt) const = 0;
    virtual std::unique_ptr<backoff_strategy> clone() const = 0;
};

// ============================================================================
// FIXED BACKOFF STRATEGY
// ============================================================================
class fixed_backoff final : public backoff_strategy
{
public:
    explicit fixed_backoff(std::chrono::milliseconds delay)
        : delay(delay)
    {
    }
    
    std::chrono::milliseconds calculate_delay(int attempt) const override
    {
        (void)attempt; // Unused
        return delay;
    }
    
    std::unique_ptr<backoff_strategy> clone() const override
    {
        return std::make_unique<fixed_backoff>(delay);
    }
    
private:
    std::chrono::milliseconds delay;
};

// ============================================================================
// EXPONENTIAL BACKOFF STRATEGY
// ============================================================================
class exponential_backoff final : public backoff_strategy
{
public:
    exponential_backoff(
        std::chrono::milliseconds initial_delay,
        double multiplier = 2.0,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60)
    )
        : initialDelay(initial_delay)
        , multiplier(multiplier)
        , maxDelay(max_delay)
    {
    }
    
    std::chrono::milliseconds calculate_delay(int attempt) const override
    {
        if (attempt <= 0)
        {
            return initialDelay;
        }
        
        auto delay_ms = initialDelay.count() * std::pow(multiplier, attempt - 1);
        auto delay = std::chrono::milliseconds(static_cast<long long>(delay_ms));
        
        return std::min(delay, maxDelay);
    }
    
    std::unique_ptr<backoff_strategy> clone() const override
    {
        return std::make_unique<exponential_backoff>(
            initialDelay, multiplier, maxDelay);
    }
    
private:
    std::chrono::milliseconds initialDelay;
    double multiplier;
    std::chrono::milliseconds maxDelay;
};

// ============================================================================
// JITTERED EXPONENTIAL BACKOFF STRATEGY
// ============================================================================
class jittered_exponential_backoff final : public backoff_strategy
{
public:
    jittered_exponential_backoff(
        std::chrono::milliseconds initial_delay,
        double multiplier = 2.0,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60),
        double jitter_factor = 0.1
    )
        : initialDelay(initial_delay)
        , multiplier(multiplier)
        , maxDelay(max_delay)
        , jitterFactor(jitter_factor)
        , gen(std::random_device{}())
    {
    }
    
    std::chrono::milliseconds calculate_delay(int attempt) const override
    {
        if (attempt <= 0)
        {
            return apply_jitter(initialDelay);
        }
        
        auto delay_ms = initialDelay.count() * std::pow(multiplier, attempt - 1);
        auto delay = std::chrono::milliseconds(static_cast<long long>(delay_ms));
        delay = std::min(delay, maxDelay);
        
        return apply_jitter(delay);
    }
    
    std::unique_ptr<backoff_strategy> clone() const override
    {
        return std::make_unique<jittered_exponential_backoff>(
            initialDelay, multiplier, maxDelay, jitterFactor);
    }
    
private:
    std::chrono::milliseconds apply_jitter(std::chrono::milliseconds delay) const
    {
        std::uniform_real_distribution<> dist(-jitterFactor, jitterFactor);
        double jitter = 1.0 + dist(gen);
        auto jittered = static_cast<long long>(delay.count() * jitter);
        return std::chrono::milliseconds(std::max(0LL, jittered));
    }
    
    std::chrono::milliseconds initialDelay;
    double multiplier;
    std::chrono::milliseconds maxDelay;
    double jitterFactor;
    mutable std::mt19937 gen;
};

// ============================================================================
// LINEAR BACKOFF STRATEGY
// ============================================================================
class linear_backoff final : public backoff_strategy
{
public:
    explicit linear_backoff(std::chrono::milliseconds increment, std::chrono::milliseconds max_delay = std::chrono::seconds(60))
        : increment(increment)
        , maxDelay(max_delay)
    {
    }
    
    std::chrono::milliseconds calculate_delay(int attempt) const override
    {
        auto delay = increment * attempt;
        return std::min(delay, maxDelay);
    }
    
    std::unique_ptr<backoff_strategy> clone() const override
    {
        return std::make_unique<linear_backoff>(increment, maxDelay);
    }
    
private:
    std::chrono::milliseconds increment;
    std::chrono::milliseconds maxDelay;
};

// ============================================================================
// RETRY POLICY
// ============================================================================
class retry_policy final
{
public:
    using retry_predicate = std::function<bool(const std::exception&, int)>;
    
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================
    
    // Default: 3 attempts with exponential backoff
    retry_policy()
        : maxAttempts(3)
        , backoff(std::make_unique<exponential_backoff>(std::chrono::milliseconds(100)))
        , retryOnAllExceptions(true)
    {
    }
    
    explicit retry_policy(int max_attempts)
        : maxAttempts(max_attempts)
        , backoff(std::make_unique<exponential_backoff>(std::chrono::milliseconds(100)))
        , retryOnAllExceptions(true)
    {
    }
    
    retry_policy(int max_attempts, std::unique_ptr<backoff_strategy> backoff)
        : maxAttempts(max_attempts)
        , backoff(std::move(backoff))
        , retryOnAllExceptions(true)
    {
    }
    
    // Copy constructor
    retry_policy(const retry_policy& other)
        : maxAttempts(other.maxAttempts)
        , backoff(other.backoff ? other.backoff->clone() : nullptr)
        , retryOnAllExceptions(other.retryOnAllExceptions)
        , retryPredicate(other.retryPredicate)
        , retryableExceptions(other.retryableExceptions)
    {
    }
    
    // Copy assignment
    retry_policy& operator=(const retry_policy& other)
    {
        if (this != &other)
        {
            maxAttempts = other.maxAttempts;
            backoff = other.backoff ? other.backoff->clone() : nullptr;
            retryOnAllExceptions = other.retryOnAllExceptions;
            retryPredicate = other.retryPredicate;
            retryableExceptions = other.retryableExceptions;
        }
        return *this;
    }
    
    // Move constructor and assignment
    retry_policy(retry_policy&&) = default;
    retry_policy& operator=(retry_policy&&) = default;
    
    // ========================================================================
    // BUILDER PATTERN METHODS
    // ========================================================================
    
    retry_policy& with_max_attempts(int attempts)
    {
        maxAttempts = attempts;
        return *this;
    }
    
    retry_policy& with_backoff(std::unique_ptr<backoff_strategy> backoffStrategy)
    {
        backoff = std::move(backoffStrategy);
        return *this;
    }
    
    retry_policy& with_fixed_backoff(std::chrono::milliseconds delay)
    {
        backoff = std::make_unique<fixed_backoff>(delay);
        return *this;
    }
    
    retry_policy& with_exponential_backoff(std::chrono::milliseconds initial_delay, double multiplier = 2.0, std::chrono::milliseconds max_delay = std::chrono::seconds(60))
    {
        backoff = std::make_unique<exponential_backoff>(initial_delay, multiplier, max_delay);
        return *this;
    }
    
    retry_policy& with_jittered_backoff(std::chrono::milliseconds initial_delay, double multiplier = 2.0, std::chrono::milliseconds max_delay = std::chrono::seconds(60), double jitter_factor = 0.1
    )
    {
        backoff = std::make_unique<jittered_exponential_backoff>(initial_delay, multiplier, max_delay, jitter_factor);
        return *this;
    }
    
    retry_policy& with_linear_backoff(std::chrono::milliseconds increment, std::chrono::milliseconds max_delay = std::chrono::seconds(60))
    {
        backoff = std::make_unique<linear_backoff>(increment, max_delay);
        return *this;
    }
    
    // ========================================================================
    // EXCEPTION HANDLING CONFIGURATION
    // ========================================================================
    
    template<typename ExceptionType>
    retry_policy& retry_on()
    {
        retryOnAllExceptions = false;
        retryableExceptions.push_back(typeid(ExceptionType).hash_code());
        return *this;
    }
    
    retry_policy& retry_on_all()
    {
        retryOnAllExceptions = true;
        retryableExceptions.clear();
        return *this;
    }
    
    retry_policy& retry_if(retry_predicate predicate)
    {
        retryPredicate = std::move(predicate);
        return *this;
    }
    
    // ========================================================================
    // EXECUTION
    // ========================================================================
    
    template<typename Func>
    auto run(Func&& func) const
    {
        using return_type = decltype(func());
        
        for (int attempt = 1; attempt <= maxAttempts; ++attempt)
        {
            try
            {
                return func();
            }
            catch (const std::exception& e)
            {
                if (!should_retry(e, attempt))
                {
                    throw;
                }
                
                if (attempt < maxAttempts)
                {
                    auto delay = backoff->calculate_delay(attempt);
                    on_retry(e, attempt, delay);
                    std::this_thread::sleep_for(delay);
                }
                else
                {
                    throw;
                }
            }
        }
        
        // Should never reach here
        throw std::runtime_error("Retry policy exhausted");
    }
    
    // ========================================================================
    // GETTERS
    // ========================================================================
    
    int get_max_attempts() const
    {
        return maxAttempts;
    }
    
    const backoff_strategy* get_backoff_strategy() const
    {
        return backoff.get();
    }
    
    // ========================================================================
    // CALLBACKS
    // ========================================================================
    
    using retry_callback = std::function<void(const std::exception&, int, std::chrono::milliseconds)>;
    
    retry_policy& on_retry(retry_callback callback)
    {
        retryCallback = std::move(callback);
        return *this;
    }
    
private:
    bool should_retry(const std::exception& e, int attempt) const
    {
        // Check if we've exhausted attempts
        if (attempt >= maxAttempts)
        {
            return false;
        }
        
        // Custom predicate takes precedence
        if (retryPredicate)
        {
            return retryPredicate(e, attempt);
        }
        
        // Check if retrying on all exceptions
        if (retryOnAllExceptions)
        {
            return true;
        }
        
        // Check if exception type is in retryable list
        auto exception_hash = typeid(e).hash_code();
        for (const auto& hash : retryableExceptions)
        {
            if (hash == exception_hash)
            {
                return true;
            }
        }
        
        return false;
    }
    
    void on_retry(const std::exception& e, int attempt, std::chrono::milliseconds delay) const
    {
        if (retryCallback)
        {
            retryCallback(e, attempt, delay);
        }
    }
    
private:
    int maxAttempts;
    std::unique_ptr<backoff_strategy> backoff;
    bool retryOnAllExceptions;
    retry_predicate retryPredicate;
    std::vector<size_t> retryableExceptions;
    retry_callback retryCallback;
};

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

// Create a simple retry policy
inline retry_policy make_retry_policy(int max_attempts)
{
    return retry_policy(max_attempts);
}

// Create retry policy with exponential backoff
inline retry_policy make_exponential_retry_policy(int max_attempts, std::chrono::milliseconds initial_delay = std::chrono::milliseconds(100))
{
    return retry_policy(max_attempts)
        .with_exponential_backoff(initial_delay);
}

// Create retry policy with jittered backoff
inline retry_policy make_jittered_retry_policy(int max_attempts, std::chrono::milliseconds initial_delay = std::chrono::milliseconds(100))
{
    return retry_policy(max_attempts)
        .with_jittered_backoff(initial_delay);
}

static const retry_policy default_retry_policy;
} // shield