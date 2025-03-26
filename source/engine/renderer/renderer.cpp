#include "renderer.h"

#include "vulkan_renderer.h"

void Renderer::init(uint32_t width, uint32_t height, SDL_Window* window)
{
	_vulkan_renderer.init(width, height, window);
}

void Renderer::process_sdl_event(const SDL_Event* sdl_event)
{
	_vulkan_renderer.process_sdl_event(sdl_event);
}

void Renderer::draw()
{
	_vulkan_renderer.draw();
}

void Renderer::cleanup()
{
	_vulkan_renderer.cleanup();
}

void Renderer::set_camera_position(const glm::vec3& position)
{
	_vulkan_renderer.set_camera_position(position);
}

void Renderer::set_camera_axes(const glm::vec3& x, const glm::vec3& y, const glm::vec3& z)
{
	_vulkan_renderer.set_camera_axes(x, y, z);
}

Renderer::RenderObjectId Renderer::add_render_object(const std::string& mesh_name, glm::mat4 transform)
{
	return _vulkan_renderer.add_render_instance(mesh_name, transform);
}

void Renderer::remove_render_object(RenderObjectId id)
{
	_vulkan_renderer.remove_render_instance(id);
}