cmake_minimum_required(VERSION 3.16)
project(TradingEngine VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------
# Engine executable
# -------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(trading_engine ${SOURCES})
target_include_directories(trading_engine
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/client/*.cpp"
)

# Add engine .cpp files that have shared logic
file(GLOB_RECURSE ENGINE_SHARED CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/swift_protocol.cpp"
    "${CMAKE_SOURCE_DIR}/src/some_other_needed_file.cpp"
)

add_executable(client ${CLIENT_SOURCES} ${ENGINE_SHARED})
target_include_directories(client
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
# -------------------------
# Tests
# -------------------------
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)
list(REMOVE_ITEM ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/tests/*.cpp"
)

add_executable(tests ${TEST_SOURCES} ${ENGINE_SOURCES})
target_include_directories(tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(tests
    PRIVATE
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)
