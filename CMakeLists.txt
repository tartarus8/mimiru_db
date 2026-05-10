# minimal cmake config
cmake_minimum_required(VERSION 3.14)

project(mimiru_db VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(SOURCES
    src/core/schema.cpp
)

set(TESTS
    tests/pager_test.cpp
)

add_executable(prog src/main.cpp ${SOURCES})
add_executable(test_pager ${TESTS} ${SOURCES})

enable_testing()

target_include_directories(prog PRIVATE src include)
target_include_directories(test_pager PRIVATE src include)
add_test(NAME PagerTest COMMAND test_pager)
