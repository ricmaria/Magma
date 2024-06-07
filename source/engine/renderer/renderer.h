#pragma once

#include <memory>
#include "vulkan_renderer.h"
#include <glm/vec3.hpp>

class Renderer
{
public:

	void init(uint32_t width, uint32_t height, struct SDL_Window* window);
	void draw();
	void cleanup();

	void set_camera_position(const glm::vec3& position);
	void set_camera_view(const glm::vec3& forward, const glm::vec3& left, const glm::vec3& up);

private:
	VulkanRenderer _vulkan_renderer;
};