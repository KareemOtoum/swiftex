cmake_minimum_required(VERSION 3.16)
project(TradingEngine VERSION 0.1 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files (all .cpp in src/)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Add executable
add_executable(trading_engine ${SOURCES})

# Add include directory
target_include_directories(trading_engine
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
