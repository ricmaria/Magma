
file(GLOB SRC_ENGINE
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

file(GLOB_RECURSE SRC_RENDERER
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/renderer/*.h"
)

message(STATUS ${SRC_ENGINE})
message(STATUS ${SRC_RENDERER})

add_library(Engine
 ${SRC_ENGINE}
 ${SRC_RENDERER}
 )

source_group("Source Files" FILES ${SRC_ENGINE})
source_group("Source Files/renderer" FILES ${SRC_RENDERER})

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/renderer/" ${Vulkan_INCLUDE_DIR})

target_link_libraries(Engine compiler_flags)

find_package(Vulkan REQUIRED)

find_package(SDL2 REQUIRED)

target_link_libraries(Engine vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(Engine Vulkan::Vulkan SDL2::SDL2main SDL2::SDL2)
