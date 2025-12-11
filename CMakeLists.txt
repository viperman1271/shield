cmake_minimum_required(VERSION 3.20)
project(shield VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(folly CONFIG REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

source_group("include\\shield" FILES
    include/shield/all.hpp
    include/shield/bulkhead.hpp
    include/shield/circuitbreaker.hpp
    include/shield/retry.hpp
    include/shield/timeoutexecutor.hpp
)

source_group("" FILES
    src/resilience_patterns.cpp
)

# Library target
add_library(shield INTERFACE
    include/shield/all.hpp
    include/shield/bulkhead.hpp
    include/shield/circuitbreaker.hpp
    include/shield/retry.hpp
    include/shield/timeoutexecutor.hpp
    src/resilience_patterns.cpp
)

target_include_directories(shield INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(shield INTERFACE
    Boost::boost
    Boost::system
    Folly::folly
    prometheus-cpp::core
)

target_compile_features(shield INTERFACE cxx_std_17)

# Example executable
add_executable(shield_example
    examples/main.cpp
)

target_link_libraries(shield_example PRIVATE
    shield
)

# Test executable
add_executable(shield_tests
    src/unittests/test_retry.cpp
    src/unittests/test_circuitbreaker.cpp
    src/unittests/test_timeout.cpp
    src/unittests/test_bulkhead.cpp
    src/unittests/test_fallback.cpp
    src/unittests/test_integration.cpp
)

target_link_libraries(shield_tests PRIVATE
    shield
    Catch2::Catch2WithMain
)

# Enable testing
enable_testing()
add_test(NAME shield_tests COMMAND shield_tests)

# Compiler warnings
if(MSVC)
    target_compile_options(shield INTERFACE /W4)
else()
    target_compile_options(shield INTERFACE -Wall -Wextra -Wpedantic)
endif()