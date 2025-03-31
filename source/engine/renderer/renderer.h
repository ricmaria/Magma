#pragma once

#include <memory>
#include "vulkan_renderer.h"
#include <glm/vec3.hpp>

union SDL_Event;

class Renderer
{
public:

	using RenderObjectId = VulkanRenderer::RenderInstanceId;

	static const RenderObjectId invalid_render_object_id = VulkanRenderer::invalid_render_instance_id;

	void init(uint32_t width, uint32_t height, struct SDL_Window* window);
	void process_sdl_event(const SDL_Event* sdl_event);
	void draw();
	void cleanup();

	void set_camera_position(const glm::vec3& position);
	void set_camera_axes(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z);

	RenderObjectId add_render_object(const std::string& mesh_name, glm::mat4 transform);
	void remove_render_object(RenderObjectId id);
	void update_render_instance(RenderObjectId id, const glm::mat4& transform);

private:
	VulkanRenderer _vulkan_renderer;
};