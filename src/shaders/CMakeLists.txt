include_guard(GLOBAL)

set(SHADER_BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(SHADER_INCLUDE_DIRS ${SHADER_BASE_DIR})
set(GLSLC_INCLUDE_OPTS "")

file(GLOB VERT_SHADER_SOURCES LIST_DIRECTORIES false ${SHADER_BASE_DIR}/vertex/*.vert)
file(GLOB FRAG_SHADER_SOURCES LIST_DIRECTORIES false ${SHADER_BASE_DIR}/fragment/*.frag)
set(SHADER_SOURCES ${VERT_SHADER_SOURCES} ${FRAG_SHADER_SOURCES})

# Create common command line options
foreach(DIR ${SHADER_INCLUDE_DIRS})
    set(GLSLC_INCLUDE_OPTS ${SHADER_INCLUDE_OPTS} -I${DIR})
endforeach()

set(GLSLC_TARGET_ENV vulkan1.2)
set(
    GLSLC_OPTIONS
        ${GLSLC_INCLUDE_OPTS}
        --target-env=${GLSLC_TARGET_ENV}
)

foreach(SRC ${SHADER_SOURCES})
    cmake_path(GET SRC FILENAME NAME)
    cmake_path(GET SRC PARENT_PATH SRC_PATH)
    cmake_path(RELATIVE_PATH SRC_PATH OUTPUT_VARIABLE REL_SRC_PATH)
    cmake_path(RELATIVE_PATH SRC OUTPUT_VARIABLE REL_SRC)

    set(OUTPUT_NAME ${NAME}.spv)
    set(OUTPUT_FILE ${REL_SRC_PATH}/${OUTPUT_NAME}) 

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        MAIN_DEPENDENCY ${SRC}
        COMMAND ${Vulkan_GLSLC_EXECUTABLE}
            ${GLSLC_OPTIONS}
            -o ${OUTPUT_FILE}
            ${SRC}
    )

    set(SPV_OUTPUTS ${SPV_OUTPUTS} ${OUTPUT_FILE})
endforeach()

add_custom_target(
    compile_shaders
    DEPENDS ${SPV_OUTPUTS}
)
