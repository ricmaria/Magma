
file(GLOB SRC_ENGINE
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

set(CORE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/core/")
file(GLOB_RECURSE SRC_CORE
    "${CORE_SOURCE_DIR}/*.cpp"
    "${CORE_SOURCE_DIR}/*.h"
)

set(RENDERER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/renderer/")
file(GLOB_RECURSE SRC_RENDERER
    "${RENDERER_SOURCE_DIR}/*.cpp"
    "${RENDERER_SOURCE_DIR}/*.h"
)

set(EC_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ec/")
file(GLOB_RECURSE SRC_EC
    "${EC_SOURCE_DIR}/*.cpp"
    "${EC_SOURCE_DIR}/*.h"
)

add_library(MagmaEngine
 ${SRC_ENGINE}
 ${SRC_CORE}
 ${SRC_RENDERER}
 ${SRC_EC}
 )

source_group("Source Files" FILES ${SRC_ENGINE})
source_group("Source Files/core" FILES ${SRC_CORE})
source_group("Source Files/renderer" FILES ${SRC_RENDERER})
source_group("Source Files/ec" FILES ${SRC_EC})

target_include_directories(MagmaEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Vulkan_INCLUDE_DIR})

target_link_libraries(MagmaEngine compiler_flags)

find_package(Vulkan REQUIRED)

find_package(SDL2 REQUIRED)

target_link_libraries(MagmaEngine vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(MagmaEngine Vulkan::Vulkan SDL2::SDL2main SDL2::SDL2)
