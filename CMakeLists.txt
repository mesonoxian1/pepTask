cmake_minimum_required(VERSION 3.20.0)

# Find Zephyr
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
message(STATUS "Zephyr base: ${ZEPHYR_BASE}")
project(app)

# Source files
target_sources(app PRIVATE 
        src/main.cpp 
        src/read/read.cpp 
        src/react/react.cpp
        hal/gpioZephyr.cpp
        )
        
# Header files
target_include_directories(app PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/read  
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/react 
        ${CMAKE_CURRENT_SOURCE_DIR}/hal 
        )