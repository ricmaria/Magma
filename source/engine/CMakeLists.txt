add_library(Engine
 engine.cpp
 engine.h
 initializers.cpp
 initializers.h
 mesh.cpp
 mesh.h
 textures.h
 textures.cpp
 types.h
 )

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Vulkan_INCLUDE_DIR})

target_link_libraries(Engine compiler_flags)

find_package(Vulkan REQUIRED)

find_package(SDL2 REQUIRED)

target_link_libraries(Engine vkbootstrap vma glm tinyobjloader imgui stb_image)

target_link_libraries(Engine Vulkan::Vulkan SDL2::SDL2main SDL2::SDL2)
