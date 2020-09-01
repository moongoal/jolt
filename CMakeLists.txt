cmake_minimum_required(VERSION 3.18)
project(
    jolt
    VERSION 0.0.1
)

include(CTest)
include(features.cmake)

find_path(PATH_SRC version.hpp.in REQUIRED DOC "Main source directory" HINTS src)
configure_file(${PATH_SRC}/version.hpp.in ${PATH_SRC}/version.hpp NEWLINE_STYLE UNIX)
configure_file(${PATH_SRC}/features.hpp.in ${PATH_SRC}/features.hpp NEWLINE_STYLE UNIX)

file(GLOB_RECURSE FILE_C_HEADERS ${PATH_SRC}/*.h)
file(GLOB_RECURSE FILE_C_SOURCES ${PATH_SRC}/*.c)
file(GLOB_RECURSE FILE_CPP_HEADERS ${PATH_SRC}/*.hpp)
file(GLOB_RECURSE FILE_CPP_SOURCES ${PATH_SRC}/*.cpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_BUILD_TYPE Debug)

add_library(
    libjolt
    SHARED
    ${FILE_C_SOURCES}
    ${FILE_CPP_SOURCES}
)

target_compile_features(
    libjolt
    PUBLIC
    cxx_std_20
    cxx_attributes
)

target_compile_definitions(libjolt PRIVATE JLT_INTERNAL)

target_include_directories(libjolt PUBLIC ${PATH_SRC})
add_subdirectory(tests/)

add_custom_command(
    TARGET libjolt POST_BUILD
    COMMAND
        ${CMAKE_COMMAND}
        -E copy_if_different
        $<TARGET_FILE:libjolt>
        $<TARGET_FILE_DIR:libjolt>/tests
)
