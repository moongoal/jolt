cmake_minimum_required(VERSION 3.18)

# vcpkg integration
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()
#vcpkg integration -- end

project(
    jolt
    VERSION 0.0.1
)

include(CTest)
include(features.cmake)

find_path(PATH_SRC_JOLT version.hpp.in REQUIRED DOC "Main source directory" HINTS src/jolt)
get_filename_component(PATH_SRC ${PATH_SRC_JOLT} DIRECTORY)

set(PATH_TESTS ${PATH_SRC}/tests)
set(PATH_SAMPLES ${PATH_SRC}/samples)

configure_file(${PATH_SRC_JOLT}/version.hpp.in ${PATH_SRC_JOLT}/version.hpp NEWLINE_STYLE UNIX)
configure_file(${PATH_SRC_JOLT}/features.hpp.in ${PATH_SRC_JOLT}/features.hpp NEWLINE_STYLE UNIX)

file(GLOB_RECURSE FILE_CPP_HEADERS ${PATH_SRC_JOLT}/*.hpp)
file(GLOB_RECURSE FILE_CPP_SOURCES ${PATH_SRC_JOLT}/*.cpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_BUILD_TYPE Debug)

add_library(
    libjolt
    SHARED
    ${FILE_CPP_SOURCES}
)

target_compile_features(
    libjolt
    PUBLIC
    cxx_std_20
    cxx_attributes
)

target_compile_definitions(libjolt
    PRIVATE
        JLT_INTERNAL
        GLM_FORCE_UNRESTRICTED_GENTYPE
        GLM_FORECE_DEPTH_ZERO_TO_ONE
    PUBLIC NOMINMAX
)

target_include_directories(libjolt PUBLIC ${PATH_SRC})

# xxHash
find_path(XXHASH_INCLUDE_DIR xxhash.h REQUIRED)
find_library(XXHASH_LIBRARY xxhash REQUIRED)

# Vulkan
include(FindVulkan)
find_package(Vulkan REQUIRED)
find_package(glm CONFIG REQUIRED)

target_include_directories(libjolt PUBLIC ${XXHASH_INCLUDE_DIR})
target_link_libraries(libjolt PRIVATE ${XXHASH_LIBRARY} PUBLIC Vulkan::Vulkan glm)

add_subdirectory(${PATH_TESTS})
add_subdirectory(${PATH_SAMPLES})

add_custom_command(
    TARGET libjolt POST_BUILD
    COMMAND
        ${CMAKE_COMMAND}
        -E copy_if_different
        $<TARGET_FILE:libjolt>
        $<TARGET_FILE_DIR:libjolt>/src/tests
)

add_custom_command(
    TARGET libjolt POST_BUILD
    COMMAND
        ${CMAKE_COMMAND}
        -E copy_if_different
        $<TARGET_FILE:libjolt>
        $<TARGET_FILE_DIR:libjolt>/src/samples
)

if(NOT ${JLT_WITH_MEM_CHECKS})
    find_file(
        SRC_MEM_CHECKS checks.cpp
        PATHS ${PATH_SRC_JOLT}/memory
        NO_DEFAULT_PATH
        REQUIRED
    )
    list(REMOVE_ITEM FILE_CPP_SOURCES ${SRC_MEM_CHECKS})
endif()

# Update the sources in case any optional feature demanded
# removal or addition of source files
set_property(TARGET libjolt PROPERTY SOURCES ${FILE_CPP_SOURCES})
