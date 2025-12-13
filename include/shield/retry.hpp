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
class fixed_backoff : public backoff_strategy
{
public:
    explicit fixed_backoff(std::chrono::milliseconds delay)
        : delay_(delay)
    {
    }
    
    std::chrono::milliseconds calculate_delay(int attempt) const override
    {
        (void)attempt; // Unused
        return delay_;
    }
    
    std::unique_ptr<backoff_strategy> clone() const override
    {
        return std::make_unique<fixed_backoff>(delay_);
    }
    
private:
    std::chrono::milliseconds delay_;
};

// ============================================================================
// EXPONENTIAL BACKOFF STRATEGY
// ============================================================================
class exponential_backoff : public backoff_strategy
{
public:
    exponential_backoff(
        std::chrono::milliseconds initial_delay,
        double multiplier = 2.0,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60)
    )
        : initial_delay_(initial_delay)
        , multiplier_(multiplier)
        , max_delay_(max_delay)
    {
    }
    
    std::chrono::milliseconds calculate_delay(int attempt) const override
    {
        if (attempt <= 0)
        {
            return initial_delay_;
        }
        
        auto delay_ms = initial_delay_.count() * std::pow(multiplier_, attempt - 1);
        auto delay = std::chrono::milliseconds(static_cast<long long>(delay_ms));
        
        return std::min(delay, max_delay_);
    }
    
    std::unique_ptr<backoff_strategy> clone() const override
    {
        return std::make_unique<exponential_backoff>(
            initial_delay_, multiplier_, max_delay_);
    }
    
private:
    std::chrono::milliseconds initial_delay_;
    double multiplier_;
    std::chrono::milliseconds max_delay_;
};

// ============================================================================
// JITTERED EXPONENTIAL BACKOFF STRATEGY
// ============================================================================
class jittered_exponential_backoff : public backoff_strategy
{
public:
    jittered_exponential_backoff(
        std::chrono::milliseconds initial_delay,
        double multiplier = 2.0,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60),
        double jitter_factor = 0.1
    )
        : initial_delay_(initial_delay)
        , multiplier_(multiplier)
        , max_delay_(max_delay)
        , jitter_factor_(jitter_factor)
        , gen_(std::random_device{}())
    {
    }
    
    std::chrono::milliseconds calculate_delay(int attempt) const override
    {
        if (attempt <= 0)
        {
            return apply_jitter(initial_delay_);
        }
        
        auto delay_ms = initial_delay_.count() * std::pow(multiplier_, attempt - 1);
        auto delay = std::chrono::milliseconds(static_cast<long long>(delay_ms));
        delay = std::min(delay, max_delay_);
        
        return apply_jitter(delay);
    }
    
    std::unique_ptr<backoff_strategy> clone() const override
    {
        return std::make_unique<jittered_exponential_backoff>(
            initial_delay_, multiplier_, max_delay_, jitter_factor_);
    }
    
private:
    std::chrono::milliseconds apply_jitter(std::chrono::milliseconds delay) const
    {
        std::uniform_real_distribution<> dist(-jitter_factor_, jitter_factor_);
        double jitter = 1.0 + dist(gen_);
        auto jittered = static_cast<long long>(delay.count() * jitter);
        return std::chrono::milliseconds(std::max(0LL, jittered));
    }
    
    std::chrono::milliseconds initial_delay_;
    double multiplier_;
    std::chrono::milliseconds max_delay_;
    double jitter_factor_;
    mutable std::mt19937 gen_;
};

// ============================================================================
// LINEAR BACKOFF STRATEGY
// ============================================================================
class linear_backoff : public backoff_strategy
{
public:
    explicit linear_backoff(
        std::chrono::milliseconds increment,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60)
    )
        : increment_(increment)
        , max_delay_(max_delay)
    {
    }
    
    std::chrono::milliseconds calculate_delay(int attempt) const override
    {
        auto delay = increment_ * attempt;
        return std::min(delay, max_delay_);
    }
    
    std::unique_ptr<backoff_strategy> clone() const override
    {
        return std::make_unique<linear_backoff>(increment_, max_delay_);
    }
    
private:
    std::chrono::milliseconds increment_;
    std::chrono::milliseconds max_delay_;
};

// ============================================================================
// RETRY POLICY
// ============================================================================
class retry_policy
{
public:
    using retry_predicate = std::function<bool(const std::exception&, int)>;
    
    // ========================================================================
    // CONSTRUCTORS
    // ========================================================================
    
    // Default: 3 attempts with exponential backoff
    retry_policy()
        : max_attempts_(3)
        , backoff_(std::make_unique<exponential_backoff>(
            std::chrono::milliseconds(100)))
        , retry_on_all_exceptions_(true)
    {
    }
    
    explicit retry_policy(int max_attempts)
        : max_attempts_(max_attempts)
        , backoff_(std::make_unique<exponential_backoff>(
            std::chrono::milliseconds(100)))
        , retry_on_all_exceptions_(true)
    {
    }
    
    retry_policy(int max_attempts, std::unique_ptr<backoff_strategy> backoff)
        : max_attempts_(max_attempts)
        , backoff_(std::move(backoff))
        , retry_on_all_exceptions_(true)
    {
    }
    
    // Copy constructor
    retry_policy(const retry_policy& other)
        : max_attempts_(other.max_attempts_)
        , backoff_(other.backoff_ ? other.backoff_->clone() : nullptr)
        , retry_on_all_exceptions_(other.retry_on_all_exceptions_)
        , retry_predicate_(other.retry_predicate_)
        , retryable_exceptions_(other.retryable_exceptions_)
    {
    }
    
    // Copy assignment
    retry_policy& operator=(const retry_policy& other)
    {
        if (this != &other)
        {
            max_attempts_ = other.max_attempts_;
            backoff_ = other.backoff_ ? other.backoff_->clone() : nullptr;
            retry_on_all_exceptions_ = other.retry_on_all_exceptions_;
            retry_predicate_ = other.retry_predicate_;
            retryable_exceptions_ = other.retryable_exceptions_;
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
        max_attempts_ = attempts;
        return *this;
    }
    
    retry_policy& with_backoff(std::unique_ptr<backoff_strategy> backoff)
    {
        backoff_ = std::move(backoff);
        return *this;
    }
    
    retry_policy& with_fixed_backoff(std::chrono::milliseconds delay)
    {
        backoff_ = std::make_unique<fixed_backoff>(delay);
        return *this;
    }
    
    retry_policy& with_exponential_backoff(
        std::chrono::milliseconds initial_delay,
        double multiplier = 2.0,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60)
    )
    {
        backoff_ = std::make_unique<exponential_backoff>(
            initial_delay, multiplier, max_delay);
        return *this;
    }
    
    retry_policy& with_jittered_backoff(
        std::chrono::milliseconds initial_delay,
        double multiplier = 2.0,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60),
        double jitter_factor = 0.1
    )
    {
        backoff_ = std::make_unique<jittered_exponential_backoff>(
            initial_delay, multiplier, max_delay, jitter_factor);
        return *this;
    }
    
    retry_policy& with_linear_backoff(
        std::chrono::milliseconds increment,
        std::chrono::milliseconds max_delay = std::chrono::seconds(60)
    )
    {
        backoff_ = std::make_unique<linear_backoff>(increment, max_delay);
        return *this;
    }
    
    // ========================================================================
    // EXCEPTION HANDLING CONFIGURATION
    // ========================================================================
    
    template<typename ExceptionType>
    retry_policy& retry_on()
    {
        retry_on_all_exceptions_ = false;
        retryable_exceptions_.push_back(typeid(ExceptionType).hash_code());
        return *this;
    }
    
    retry_policy& retry_on_all()
    {
        retry_on_all_exceptions_ = true;
        retryable_exceptions_.clear();
        return *this;
    }
    
    retry_policy& retry_if(retry_predicate predicate)
    {
        retry_predicate_ = std::move(predicate);
        return *this;
    }
    
    // ========================================================================
    // EXECUTION
    // ========================================================================
    
    template<typename Func>
    auto run(Func&& func) const
    {
        using return_type = decltype(func());
        
        for (int attempt = 1; attempt <= max_attempts_; ++attempt)
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
                
                if (attempt < max_attempts_)
                {
                    auto delay = backoff_->calculate_delay(attempt);
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
        return max_attempts_;
    }
    
    const backoff_strategy* get_backoff_strategy() const
    {
        return backoff_.get();
    }
    
    // ========================================================================
    // CALLBACKS
    // ========================================================================
    
    using retry_callback = std::function<void(
        const std::exception&, int, std::chrono::milliseconds)>;
    
    retry_policy& on_retry(retry_callback callback)
    {
        retry_callback_ = std::move(callback);
        return *this;
    }
    
private:
    bool should_retry(const std::exception& e, int attempt) const
    {
        // Check if we've exhausted attempts
        if (attempt >= max_attempts_)
        {
            return false;
        }
        
        // Custom predicate takes precedence
        if (retry_predicate_)
        {
            return retry_predicate_(e, attempt);
        }
        
        // Check if retrying on all exceptions
        if (retry_on_all_exceptions_)
        {
            return true;
        }
        
        // Check if exception type is in retryable list
        auto exception_hash = typeid(e).hash_code();
        for (const auto& hash : retryable_exceptions_)
        {
            if (hash == exception_hash)
            {
                return true;
            }
        }
        
        return false;
    }
    
    void on_retry(const std::exception& e, int attempt, 
                  std::chrono::milliseconds delay) const
    {
        if (retry_callback_)
        {
            retry_callback_(e, attempt, delay);
        }
    }
    
    int max_attempts_;
    std::unique_ptr<backoff_strategy> backoff_;
    bool retry_on_all_exceptions_;
    retry_predicate retry_predicate_;
    std::vector<size_t> retryable_exceptions_;
    retry_callback retry_callback_;
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
inline retry_policy make_exponential_retry_policy(
    int max_attempts,
    std::chrono::milliseconds initial_delay = std::chrono::milliseconds(100)
)
{
    return retry_policy(max_attempts)
        .with_exponential_backoff(initial_delay);
}

// Create retry policy with jittered backoff
inline retry_policy make_jittered_retry_policy(
    int max_attempts,
    std::chrono::milliseconds initial_delay = std::chrono::milliseconds(100)
)
{
    return retry_policy(max_attempts)
        .with_jittered_backoff(initial_delay);
}

template<typename Func>
auto retry(Func&& func, int max_attempts = 3, std::chrono::milliseconds delay = std::chrono::milliseconds(100))
{
    using return_type = decltype(func());

    for (int attempt = 1; attempt <= max_attempts; ++attempt)
    {
        try
        {
            return func();
        }
        catch (const std::exception& e)
        {
            if (attempt == max_attempts)
            {
                throw;
            }
            std::cout << "Attempt " << attempt << " failed: " << e.what()
                << ". Retrying...\n";
            std::this_thread::sleep_for(delay);
            delay *= 2; // Exponential backoff
        }
    }

    throw std::runtime_error("Retry failed");
}

static const retry_policy default_retry_policy;
} // shield