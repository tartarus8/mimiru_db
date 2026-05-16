FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(mimiru_tests
    pager_test.cpp
    bplus_tree_test.cpp
    engine_test.cpp
    lexer_test.cpp
    parser_test.cpp
)

target_link_libraries(mimiru_tests PRIVATE
    gtest_main
    mimiru_lib
)


include(GoogleTest)
gtest_discover_tests(mimiru_tests)
