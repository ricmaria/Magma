cmake_minimum_required(VERSION 3.8...3.31.3)

project(Magma VERSION 0.1.0)

find_package(Vulkan REQUIRED)

add_subdirectory(third_party)

set(current_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
cmake_path(GET current_source_dir PARENT_PATH parent_dir)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${parent_dir}/bin")

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_20)

add_subdirectory(engine)

add_executable(MagmaGame main.cpp)
target_sources(MagmaGame PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glm.natvis)

# add_source_group(Visualizers "Visualizers" FILES my_project.natvis)

target_link_libraries(MagmaGame PUBLIC MagmaEngine compiler_flags)

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${parent_dir}/shaders/*.frag"
    "${parent_dir}/shaders/*.vert"
    "${parent_dir}/shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${parent_dir}/shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )
	
add_custom_command(TARGET MagmaGame POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:MagmaGame> $<TARGET_FILE_DIR:MagmaGame>
  COMMAND_EXPAND_LISTS
  )