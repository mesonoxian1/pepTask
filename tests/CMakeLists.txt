cmake_minimum_required(VERSION 3.20.0)
project(gpio_tests VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Fetch GTest — no manual install needed
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# App logic under test
# Compiles read.cpp and react.cpp against Zephyr stubs instead of real SDK
# ---------------------------------------------------------------------------
add_library(app_logic STATIC
    ${CMAKE_SOURCE_DIR}/../src/read/read.cpp
    ${CMAKE_SOURCE_DIR}/../src/react/react.cpp
    zbus_chan_stub.cpp
)
target_include_directories(app_logic PUBLIC
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../src/read
    ${CMAKE_SOURCE_DIR}/../src/react
    ${CMAKE_SOURCE_DIR}/stubs/include
)

# ---------------------------------------------------------------------------
# Test executable
# ---------------------------------------------------------------------------
add_executable(gpio_tests
    test_read_class.cpp
    test_react_class.cpp
)
target_include_directories(gpio_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../src/read
    ${CMAKE_SOURCE_DIR}/../src/react
    ${CMAKE_SOURCE_DIR}/stubs/include
    ${CMAKE_SOURCE_DIR}/mocks
)
target_link_libraries(gpio_tests PRIVATE
    app_logic
    GTest::gtest_main
    GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(gpio_tests)
