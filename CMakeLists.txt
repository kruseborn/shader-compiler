cmake_minimum_required(VERSION 3.12)

project(ShaderCompiler)

include(cmake-modules/copts.cmake)
include(cmake-modules/platform.cmake)
include(cmake-modules/helper.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_subdirectory(glsl-compiler)
add_subdirectory(spirv-reflection)
add_subdirectory(spirv-cross)