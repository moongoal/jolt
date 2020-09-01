get_filename_component(
    DIR_PROJECT
    ${CMAKE_CURRENT_LIST_DIR}
    DIRECTORY
)
set(DIR_BUILD ${DIR_PROJECT}/build)

execute_process(
    COMMAND ${CMAKE_COMMAND} -S ${DIR_PROJECT} -B ${DIR_BUILD} -G Ninja
    WORKING_DIRECTORY ${DIR_PROJECT}
    RESULT_VARIABLE RESULT_INIT_CMAKE
)

if (NOT RESULT_INIT_CMAKE EQUAL 0)
    message(FATAL_ERROR "Initialization failed")
else()
    find_file(FILE_COMPILE_COMMANDS compile_commands.json HINTS ${DIR_BUILD} REQUIRED)
    file(COPY ${FILE_COMPILE_COMMANDS} DESTINATION ${DIR_PROJECT})
endif()
