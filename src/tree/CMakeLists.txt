add_library(bplus_tree
    src/bplus_tree.cpp
)

target_include_directories(bplus_tree
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BUILD_TESTING)
    include(FetchContent)

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(googletest)

    add_executable(bplus_tree_tests
        tests/bplus_tree_tests.cpp
    )

    target_link_libraries(bplus_tree_tests
        PRIVATE
            bplus_tree
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_add_tests(TARGET bplus_tree_tests)
endif()
