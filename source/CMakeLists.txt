cmake_minimum_required(VERSION 3.28)

project(Magma VERSION 0.1.0)

add_subdirectory(third_party)

set(current_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
cmake_path(GET current_source_dir PARENT_PATH parent_dir)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${parent_dir}/bin")

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_11)

add_subdirectory(engine)

add_executable(Magma main.cpp)

target_link_libraries(Magma PUBLIC Engine compiler_flags)