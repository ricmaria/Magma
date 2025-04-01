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

	void set_camera_transform(const glm::mat4x4& transform);

	RenderObjectId add_render_object(const std::string& mesh_name, glm::mat4 transform);
	void remove_render_object(RenderObjectId id);
	void update_render_object(RenderObjectId id, const glm::mat4& transform);

private:
	VulkanRenderer _vulkan_renderer;
};