// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <types.h>
#include <renderer.h>

namespace vkutil {

	bool load_image_from_file(VulkanRenderer& renderer, const char* file, AllocatedImage& outImage);

}