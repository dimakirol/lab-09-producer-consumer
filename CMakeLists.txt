cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(BUILD_COVERAGE "Build coverage" OFF)

set(Boost_Path "/home/kirill/Documents/boost")
set(Boost_INCLUDE_DIR "${Boost_Path}/installed/include")
string(APPEND CMAKE_CXX_FLAGS " -g -Wall -DBOOST_LOG_DYN_LINK")
set(D_LIBRARY_PATH="/home/kirill/Documents//boost/installed/lib/")

set(
        HUNTER_CACHE_SERVERS
        "https://github.com/bmstu-iu8-cpp-sem-3/hunter-cache"
        CACHE STRING "Default cache server"
)
include("tools/gate/cmake/HunterGate.cmake")
include_directories(include)

HunterGate(
        URL "https://github.com/cpp-pm/hunter/archive/v0.23.218.tar.gz"
        SHA1 "9a3594a61227967fb92159339ba326701b287b19"
)

project(9_lab)

find_package(Boost 1.72.0 REQUIRED COMPONENTS program_options log log_setup thread)

hunter_add_package(Beast)
find_package(Beast CONFIG REQUIRED)

hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

hunter_add_package(Gumbo)
find_package(gumbo CONFIG REQUIRED)

add_executable(9_lab
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/source.cpp
)

target_include_directories(9_lab 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(9_lab ${Boost_LIBRARIES} Beast::Beast OpenSSL::SSL OpenSSL::Crypto)

link_directories("${Boost_Path}/installed/lib")
