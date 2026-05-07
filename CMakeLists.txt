# minimal cmake config
cmake_minimum_required(VERSION 3.14)

project(mimiru_db VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(prog ${SOURCES})

target_include_directories(prog PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
