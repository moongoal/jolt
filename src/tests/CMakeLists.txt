include_guard(GLOBAL)
file(GLOB TEST_SOURCES LIST_DIRECTORIES false ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

foreach(SRC ${TEST_SOURCES})
    cmake_path(GET SRC STEM NAME)
    add_executable(${NAME} ${SRC})
    target_link_libraries(${NAME} libjolt)
    add_test(${NAME} ${NAME})
    add_custom_command(
        TARGET ${NAME} POST_BUILD
        COMMAND
            ${CMAKE_COMMAND}
            -E copy_if_different
            $<TARGET_FILE:libjolt>
            $<TARGET_FILE_DIR:${NAME}>
    )
    set_tests_properties(
        ${NAME}
        PROPERTIES
            PASS_REGULAR_EXPRESSION "PASS"
            FAIL_REGULAR_EXPRESSION "FAIL"
    )
endforeach()
