cmake_minimum_required(VERSION 3.4)
project(source)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(BUILD_COVERAGE "Build coverage" OFF)

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

string(APPEND CMAKE_CXX_FLAGS " -pedantic -Werror -Wall -Wextra")
string(APPEND CMAKE_CXX_FLAGS " -Wno-unused-command-line-argument")
string(APPEND CMAKE_CXX_FLAGS " -Wshadow -Wnon-virtual-dtor")

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

hunter_add_package(Boost COMPONENTS thread log system filesystem program_options)
find_package(Boost CONFIG REQUIRED thread log system filesystem program_options)

hunter_add_package(Beast)
find_package(Beast CONFIG REQUIRED)

hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

hunter_add_package(Gumbo)
find_package(gumbo CONFIG REQUIRED)

add_library(source STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/source.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/attribute.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/char_ref.c 
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/error.c 
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/string_buffer.c 
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/string_piece.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/tag.c 
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/tokenizer.c 
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/utf8.c 
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/util.c
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src/vector.c 
)

add_executable(tests
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/source.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.cpp
)

add_executable(main
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/source.cpp
)

target_include_directories(source 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/ThreadPool
)

target_include_directories(tests 
 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src
 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/ThreadPool
)

target_include_directories(main 
 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/gumbo-parser/src
 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third-party/ThreadPool
)

target_link_libraries(source Boost::boost Beast::Beast Boost::program_options OpenSSL::SSL OpenSSL::Crypto Boost::thread Boost::system Boost::log Boost::filesystem source)
target_link_libraries(tests GTest::main Boost::boost Beast::Beast Boost::program_options OpenSSL::SSL OpenSSL::Crypto Boost::thread Boost::system Boost::log Boost::filesystem source)
target_link_libraries(source Boost::boost Beast::Beast OpenSSL::SSL OpenSSL::Crypto Boost::program_options Boost::thread Boost::system Boost::log Boost::filesystem)

enable_testing()
add_test(NAME unit_tests COMMAND tests) 

endif()