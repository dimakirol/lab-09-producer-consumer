cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(
        HUNTER_CACHE_SERVERS
        "https://github.com/bmstu-iu8-cpp-sem-3/hunter-cache"
        CACHE STRING "Default cache server"
)
include("tools/gate/cmake/HunterGate.cmake")


HunterGate(
        URL "https://github.com/cpp-pm/hunter/archive/v0.23.218.tar.gz"
        SHA1 "9a3594a61227967fb92159339ba326701b287b19"
)

project(9_lab)

option(BUILD_COVERAGE "Build coverage" OFF)

#set(Boost_Path "/home/kirill/Documents/boost")
#set(Boost_INCLUDE_DIR "${Boost_Path}/installed/include")
#string(APPEND CMAKE_CXX_FLAGS " -g -Wall -DBOOST_LOG_DYN_LINK")
#set(D_LIBRARY_PATH="/home/kirill/Documents//boost/installed/lib/")
#include_directories(include)

#set(BEAST_INCLUDE_DIR ${Boost_INCLUDE_DIR}/beast)
#include_directories(${BEAST_INCLUDE_DIR})

#find_package(Boost 1.72.0 REQUIRED COMPONENTS program_options coroutine log log_setup thread)
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

hunter_add_package(gumbo)
find_package(gumbo CONFIG REQUIRED)

hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

hunter_add_package(Boost COMPONENTS program_options coroutine log log_setup thread)
find_package(Boost 1.70.0 COMPONENTS REQUIRED program_options coroutine log log_setup thread)

add_library(9_lab STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/source.cpp
        )
#add_executable(9_lab
#        ${CMAKE_CURRENT_SOURCE_DIR}/sources/source.cpp
#        )

add_executable(tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp
        )

target_include_directories(9_lab
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

target_include_directories(tests
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

target_link_libraries(tests GTest::main 9_lab gumbo::gumbo OpenSSL::SSL)
target_link_libraries(9_lab Boost::program_options Boost::log Boost::thread)

#include_directories(${Boost_INCLUDE_DIRS})
#target_link_libraries(9_lab ${Boost_LIBRARIES} gumbo::gumbo OpenSSL::SSL)
#link_directories("${Boost_Path}/installed/lib")

enable_testing()
add_test(NAME unit_tests COMMAND tests)

if(BUILD_COVERAGE)
    set(ENABLE_COVERAGE ON CACHE BOOL "Enable coverage build." FORCE)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    find_package(codecov)
    add_coverage(cache)
    add_coverage(tests)
    list(APPEND LCOV_REMOVE_PATTERNS "'${PROJECT_SOURCE_DIR}/tests/*'")
    coverage_evaluate()
endif()