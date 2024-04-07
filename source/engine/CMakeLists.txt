add_library(Engine
 engine.cpp
 engine.h
 types.h
 initializers.cpp
 initializers.h)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Vulkan_INCLUDE_DIR})

target_link_libraries(Engine PUBLIC compiler_flags)

find_package(Vulkan REQUIRED)

find_package(SDL2 REQUIRED)

target_link_libraries(Engine PRIVATE Vulkan::Vulkan SDL2::SDL2main SDL2::SDL2)
