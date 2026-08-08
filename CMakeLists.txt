cmake_minimum_required(VERSION 3.20)
project(HighPerfHttpServer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Strict warnings to catch issues early
add_compile_options(
    -Wall -Wextra -Wpedantic -Wshadow
    -Wconversion -Wsign-conversion -Wcast-align
)

# Optional sanitizers for debug builds to catch memory leaks and UB
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" ON)
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Core static library
add_library(server_core STATIC
    src/network/Socket.cpp
    src/server/Listener.cpp
    src/config/Config.cpp
    src/utils/Logger.cpp
    src/server/EventLoop.cpp
    src/server/Connection.cpp
    src/server/ConnectionManager.cpp
    src/server/TimerManager.cpp
    src/network/Epoll.cpp
    src/concurrency/ThreadPool.cpp
    src/http/HttpParser.cpp
    src/http/HttpResponse.cpp
    src/routing/Router.cpp
    src/server/App.cpp
    src/middleware/StaticFiles.cpp
)

add_library(HttpServer::core ALIAS server_core)

target_include_directories(server_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include/http-server>
)

# Install Rules for the Framework
include(GNUInstallDirs)

install(TARGETS server_core
    EXPORT HttpServerTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/http-server
)

install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/http-server
    FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT HttpServerTargets
    FILE HttpServerTargets.cmake
    NAMESPACE HttpServer::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HttpServer
)

# Core executable
add_executable(http_server src/main.cpp)
target_link_libraries(http_server PRIVATE server_core)

# Testing Setup (GoogleTest via FetchContent)
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(http_server_tests 
    tests/test_dummy.cpp
    tests/test_parser.cpp
    tests/test_router.cpp
    tests/test_threadpool.cpp
)
target_link_libraries(http_server_tests PRIVATE gtest_main server_core)
include(GoogleTest)
gtest_discover_tests(http_server_tests)
