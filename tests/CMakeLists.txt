add_executable(test_pager pager_test.cpp ${CORE_SOURCES})
target_include_directories(test_pager PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)
add_test(NAME PagerTest COMMAND test_pager)
