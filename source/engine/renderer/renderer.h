#pragma once

#include <memory>
#include "core/reflectable.h"
#include "vulkan_renderer.h"
#include <glm/vec3.hpp>

class Renderer : public Reflectable
{
public:
	Renderer()
	{
		register_my_type<decltype(*this)>();
	}

	void init(uint32_t width, uint32_t height, struct SDL_Window* window);
	void draw();
	void cleanup();

	void set_camera_position(const glm::vec3& position);

private:
	VulkanRenderer _vulkan_renderer;
};