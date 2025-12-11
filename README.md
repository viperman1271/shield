# Shield C++ 🛡️

A modern C++ resilience library implementing essential fault-tolerance patterns for building robust distributed systems.

## Features

- **Retry Pattern**: Automatic retry with exponential backoff
- **Circuit Breaker**: Prevent cascading failures with state management
- **Timeout Pattern**: Time-bound operations with multiple implementations
- **Bulkhead Pattern**: Resource isolation and concurrent execution limits
- **Fallback Pattern**: Graceful degradation with alternative strategies
- **Observability**: Built-in Prometheus metrics integration

## Requirements

- C++17 or higher
- CMake 3.20+
- vcpkg (recommended for dependency management)

## Dependencies

- Boost (system, asio)
- Folly
- Prometheus C++ client
- Catch2 (for testing)

## Installation

### Using vcpkg

```bash
# Install vcpkg if you haven't already
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Install dependencies
./vcpkg install boost-system boost-asio folly prometheus-cpp catch2

# Build the project
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

### Manual Installation

Install dependencies using your system package manager, then:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Quick Start

```cpp
#include "shield_cpp.hpp"

using namespace shield_cpp;

// Retry with exponential backoff
auto result = retry([]() {
    return call_unreliable_service();
}, 3, std::chrono::milliseconds(100));

// Circuit breaker to prevent cascading failures
circuit_breaker cb(5, std::chrono::seconds(60));
auto data = cb.execute([]() {
    return fetch_data_from_service();
});

// Timeout for long-running operations
auto response = with_timeout([]() {
    return slow_network_call();
}, std::chrono::seconds(5));

// Fallback for graceful degradation
auto value = with_fallback(
    []() { return fetch_from_primary(); },
    []() { return fetch_from_cache(); }
);

// Combine patterns for maximum resilience
auto resilient_call = with_fallback(
    [&cb]() {
        return cb.execute([]() {
            return retry([]() {
                return with_timeout([]() {
                    return api_call();
                }, std::chrono::seconds(2));
            }, 3);
        });
    },
    []() { return cached_response(); }
);
```

## Pattern Details

### Retry Pattern

Automatically retries failed operations with configurable attempts and exponential backoff.

```cpp
int attempt = 0;
auto result = retry([&attempt]() {
    attempt++;
    if (attempt < 3) {
        throw std::runtime_error("Temporary failure");
    }
    return 42;
}, 5, std::chrono::milliseconds(100));
```

### Circuit Breaker Pattern

Implements the circuit breaker pattern with three states: CLOSED, OPEN, and HALF_OPEN.

```cpp
circuit_breaker cb(5, std::chrono::seconds(60));

// Circuit opens after 5 failures
for (int i = 0; i < 5; i++) {
    try {
        cb.execute([]() {
            throw std::runtime_error("Service down");
            return 0;
        });
    } catch (...) {}
}

// Circuit is now OPEN - calls fail fast
assert(cb.get_state() == circuit_breaker::state::open);
```

### Timeout Pattern

Provides time-bounded execution with two implementations:

```cpp
// Simple timeout using std::async
auto result = with_timeout([]() {
    return long_running_operation();
}, std::chrono::seconds(5));

// Advanced timeout using Boost.Asio
timeout_executor executor;
auto result = executor.execute_with_timeout([]() {
    return operation();
}, std::chrono::milliseconds(500));
```

### Bulkhead Pattern

Limits concurrent executions to prevent resource exhaustion.

```cpp
bulkhead bh(10); // Max 10 concurrent operations

auto future = bh.execute([]() {
    return expensive_operation();
});

auto result = future.get();
```

### Fallback Pattern

Provides alternative execution paths when primary operations fail.

```cpp
auto result = with_fallback(
    []() { return primary_service(); },
    []() { return cached_value(); }
);
```

## Testing

Comprehensive unit tests are provided using Catch2:

```bash
# Run all tests
./build/shield_tests

# Run specific test suites
./build/shield_tests "[retry]"
./build/shield_tests "[circuit_breaker]"
./build/shield_tests "[integration]"
```

## Observability

Shield C++ integrates with Prometheus for monitoring:

```cpp
auto registry = std::make_shared<prometheus::Registry>();
resilient_service service(registry);

// Automatic metrics collection:
// - requests_total
// - failures_total
// - active_requests

auto result = service.execute_resilient(
    []() { return api_call(); },
    []() { return fallback_value(); }
);
```

## Project Structure

```
shield-cpp/
├── include/
│   └── shield_cpp.hpp          # Main library header
├── examples/
│   └── main.cpp                # Usage examples
├── tests/
│   ├── test_retry.cpp          # Retry pattern tests
│   ├── test_circuit_breaker.cpp # Circuit breaker tests
│   ├── test_timeout.cpp        # Timeout pattern tests
│   ├── test_bulkhead.cpp       # Bulkhead pattern tests
│   ├── test_fallback.cpp       # Fallback pattern tests
│   └── test_integration.cpp    # Integration tests
├── CMakeLists.txt              # Build configuration
├── vcpkg.json                  # Dependency manifest
└── README.md                   # This file
```

## Best Practices

1. **Combine Patterns**: Use multiple patterns together for comprehensive resilience
2. **Set Appropriate Timeouts**: Balance between giving operations time to complete and failing fast
3. **Monitor Metrics**: Use Prometheus integration to track system health
4. **Test Failure Scenarios**: Ensure fallback paths are tested and reliable
5. **Configure Circuit Breakers**: Tune thresholds based on your service's error rates

## Contributing

Contributions are welcome! Please ensure:

- Code follows the existing style (snake_case, braces on separate lines)
- All tests pass
- New features include comprehensive tests
- Documentation is updated

## License

MIT License - see LICENSE file for details

## Acknowledgments

Built with industry-leading libraries:
- [Boost](https://www.boost.org/)
- [Folly](https://github.com/facebook/folly)
- [Prometheus C++](https://github.com/jupp0r/prometheus-cpp)
- [Catch2](https://github.com/catchorg/Catch2)
