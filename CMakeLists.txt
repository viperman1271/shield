cmake_minimum_required(VERSION 3.20)
project(shield)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    include/shield/fallback.hpp
    include/shield/retry.hpp
    include/shield/timeout.hpp
)

source_group("" FILES
    src/resilience_patterns.cpp
)

source_group("circuit" FILES
    src/circuit/circuit.cpp
    src/circuit/circuitbreaker.cpp
    src/circuit/circuitbreakermanager.cpp
)

# Library target
add_library(shield STATIC
    include/shield/all.hpp
    include/shield/bulkhead.hpp
    include/shield/circuitbreaker.hpp
    include/shield/fallback.hpp
    include/shield/retry.hpp
    include/shield/timeout.hpp
    src/circuit/circuit.cpp
    src/circuit/circuitbreaker.cpp
    src/circuit/circuitbreakermanager.cpp
    src/resilience_patterns.cpp
)

target_include_directories(shield PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(shield PUBLIC
    Boost::boost
    Boost::system
    Folly::folly
    prometheus-cpp::core
)

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

if(MSVC)
    target_compile_definitions(shield PUBLIC _WIN32_WINNT=0x0601)
    target_compile_options(shield PUBLIC /W4 /utf-8)
else()
    target_compile_options(shield PRIVATE -Wall -Wextra -Wpedantic)
endif()