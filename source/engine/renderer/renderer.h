#pragma once

#include <memory>
#include "vulkan_renderer.h"
#include <glm/vec3.hpp>

union SDL_Event;

class Renderer
{
public:

	using RenderObjectId = VulkanRenderer::RenderObjectId;

	static const RenderObjectId invalid_render_object_id = VulkanRenderer::invalid_render_object_id;

	void init(uint32_t width, uint32_t height, struct SDL_Window* window);
	void process_sdl_event(const SDL_Event* sdl_event);
	void draw();
	void cleanup();

	void set_camera_position(const glm::vec3& position);
	void set_camera_view(const glm::vec3& forward, const glm::vec3& left, const glm::vec3& up);

	RenderObjectId add_render_object(const std::string& mesh_name, const std::string& material_name, glm::mat4 transform);
	void remove_render_object(RenderObjectId id);

private:
	VulkanRenderer _vulkan_renderer;
};