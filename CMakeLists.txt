cmake_minimum_required(VERSION 3.14)
project(mimiru_db LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")


include(FetchContent)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_library(mimiru_lib)


target_include_directories(mimiru_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_sources(mimiru_lib PRIVATE
    src/storage/pager.cpp
    src/index/bplus_tree.cpp
    src/engine/table.cpp
    src/engine/system_manager.cpp
    src/engine/database.cpp
    src/query/lexer.cpp
    src/query/parser.cpp
)

target_link_libraries(mimiru_lib PUBLIC nlohmann_json::nlohmann_json)

add_executable(prog src/main.cpp)
target_link_libraries(prog PRIVATE mimiru_lib)

enable_testing()
add_subdirectory(tests)
