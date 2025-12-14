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

find_path(ITLIB_INCLUDE_DIRS "itlib/any.hpp")

# Need to override some of the Folly properties - specifically the C++17 command line
set_target_properties(Folly::folly PROPERTIES
  INTERFACE_COMPILE_OPTIONS "/EHs;/GF;/Zc:referenceBinding;/Zc:rvalueCast;/Zc:implicitNoexcept;/Zc:strictStrings;/Zc:threadSafeInit;/Zc:throwingNew;/permissive-;/std:c++20;/utf-8;/wd4191;/wd4291;/wd4309;/wd4310;/wd4366;/wd4587;/wd4592;/wd4628;/wd4723;/wd4724;/wd4868;/wd4996;/wd4068;/wd4091;/wd4146;/wd4800;/wd4018;/wd4365;/wd4388;/wd4389;/wd4100;/wd4459;/wd4505;/wd4701;/wd4702;/wd4061;/wd4127;/wd4200;/wd4201;/wd4296;/wd4316;/wd4324;/wd4355;/wd4371;/wd4435;/wd4514;/wd4548;/wd4571;/wd4574;/wd4582;/wd4583;/wd4619;/wd4623;/wd4625;/wd4626;/wd4643;/wd4647;/wd4668;/wd4706;/wd4710;/wd4711;/wd4714;/wd4820;/wd5026;/wd5027;/wd5031;/wd5045;/we4099;/we4129;/we4566"
)

source_group("include\\shield" FILES
    include/shield/all.hpp
    include/shield/bulkhead.hpp
    include/shield/circuit.hpp
    include/shield/circuitbreaker.hpp
    include/shield/exceptions.hpp
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

source_group("defailt\\circuit" FILES
    src/detail/circuit/circuitbreakermanager.cpp
    src/detail/circuit/circuitbreakermanager.hpp
)

# Library target
add_library(shield STATIC
    include/shield/all.hpp
    include/shield/bulkhead.hpp
    include/shield/circuit.hpp
    include/shield/circuitbreaker.hpp
    include/shield/exceptions.hpp
    include/shield/fallback.hpp
    include/shield/retry.hpp
    include/shield/timeout.hpp
    src/circuit/circuit.cpp
    src/circuit/circuitbreaker.cpp
    src/detail/circuit/circuitbreakermanager.cpp
    src/detail/circuit/circuitbreakermanager.hpp
    src/resilience_patterns.cpp
)

target_include_directories(shield PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${ITLIB_INCLUDE_DIRS}
)

target_include_directories(shield PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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
    src/unittests/test_circuit.cpp
    src/unittests/test_circuitbreaker.cpp
    src/unittests/test_timeout.cpp
    src/unittests/test_bulkhead.cpp
    src/unittests/test_fallback.cpp
    src/unittests/test_integration.cpp
)

target_include_directories(shield_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
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