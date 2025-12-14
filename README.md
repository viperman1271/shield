# Shield C++ 🛡️

A modern C++ resilience library implementing essential fault-tolerance patterns for building robust distributed systems.

# Features

- **Retry Pattern**: Automatic retry with exponential backoff
- **Circuit Breaker**: Prevent cascading failures with state management
- **Timeout Pattern**: Time-bound operations with multiple implementations
- **Bulkhead Pattern**: Resource isolation and concurrent execution limits
- **Fallback Pattern**: Graceful degradation with alternative strategies
- **Observability**: Built-in Prometheus metrics integration

# Requirements

- C++20 or higher
- CMake 3.10+
- vcpkg

# How to Build

## Install vcpkg

If vcpkg is not already present and available system-wide, you'll need to setup vcpkg

```bash
# Install vcpkg if you haven't already
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# Ensure env values are available - should write the path where it was installed
echo $VCPKG_ROOT
```

## Run CMake
Here are examples on running CMake with system-wide vcpkg installation

### Windows
```bash
# Configure
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -S . -G "Visual Studio 17 2022"

# Build
cmake --build build --config Debug -j4
```

Will specifically create for Visual Studio 2022. This will compile the solution from the build folder, in Debug configuration, with 4 threads.

### Linux
```bash
# Configure
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -S .

# Build
cmake --build build --config Debug -j4
```

This will compile the application from the build folder, in Debug configuration, with 4 threads.

# Project Structure

```
shield/
├── include/
|   └── shield/                 # All public library headers
├── examples/
│   └── main.cpp                # Usage examples
├── src/ 
│   ├── detail/                 # Any private classes
|   └── tests/                  # Unit tests
├── CMakeLists.txt              # Build configuration
├── vcpkg.json                  # Dependency manifest
└── README.md                   # This file
```

# Best Practices

1. **Combine Patterns**: Use multiple patterns together for comprehensive resilience
2. **Set Appropriate Timeouts**: Balance between giving operations time to complete and failing fast
3. **Monitor Metrics**: Use Prometheus integration to track system health
4. **Test Failure Scenarios**: Ensure fallback paths are tested and reliable
5. **Configure Circuit Breakers**: Tune thresholds based on your service's error rates

# Contributing

Contributions are welcome! Please ensure:

- Code follows the existing style
- All tests pass
- New features include comprehensive tests
- Documentation is updated

# License

MIT License - see [LICENSE](LICENSE) file for details

# Dependencies

Built with the following libraries:
- [Boost](https://www.boost.org/)
- [Folly](https://github.com/facebook/folly)
- [itlib](https://github.com/iboB/itlib)
- [Prometheus C++](https://github.com/jupp0r/prometheus-cpp)
- [Catch2](https://github.com/catchorg/Catch2)
