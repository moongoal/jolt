include_guard(GLOBAL)
file(GLOB SAMPLE_SOURCES LIST_DIRECTORIES false ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

foreach(SRC ${SAMPLE_SOURCES})
    get_filename_component(NAME ${SRC} NAME_WE ABSOLUTE)
    add_executable(${NAME} ${SRC})
    target_link_libraries(${NAME} libjolt)
    add_custom_command(
        TARGET ${NAME} POST_BUILD
        COMMAND
            ${CMAKE_COMMAND}
            -E copy_if_different
            $<TARGET_FILE:libjolt>
            $<TARGET_FILE_DIR:${NAME}>
    )
endforeach()
