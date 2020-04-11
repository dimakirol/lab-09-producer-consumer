cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_minimum_required(VERSION 3.15)
project(8_lab)

option(BUILD_COVERAGE "Build coverage" OFF)
set(CMAKE_CXX_STANDARD 14)

set(Boost_Path "/home/kirill/Documents/boost")
set(Boost_INCLUDE_DIR "${Boost_Path}/installed/include")
string(APPEND CMAKE_CXX_FLAGS " -g -Wall -DBOOST_LOG_DYN_LINK")
set(D_LIBRARY_PATH="/home/kirill/Documents//boost/installed/lib/")
include_directories(include)

find_package(Boost 1.72.0 COMPONENTS REQUIRED thread log log_setup)

add_executable(8_lab
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/source.cpp
        )

target_include_directories(8_lab
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(8_lab ${Boost_LIBRARIES})

link_directories("${Boost_Path}/installed/lib")
