add_subdirectory(tree)

add_executable(prog
    main.cpp
    core/schema.cpp
    engine/table.cpp
    query/executor.cpp
    query/parser.cpp
    storage/file_storage.cpp
)

target_include_directories(prog
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(prog
    PRIVATE
        bplus_tree
)
