cmake_minimum_required(VERSION 3.20.0)

# Find Zephyr
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
message(STATUS "Zephyr base: ${ZEPHYR_BASE}")
project(app)

# Add your source files
target_sources(app PRIVATE src/main.cpp)