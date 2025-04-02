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

void Renderer::set_camera_transform(const Transform& transform)
{
	_vulkan_renderer.set_camera_transform(transform);
}

Renderer::RenderObjectId Renderer::add_render_object(const std::string& mesh_name, glm::mat4 transform)
{
	return _vulkan_renderer.add_render_object(mesh_name, transform);
}

void Renderer::remove_render_object(RenderObjectId id)
{
	_vulkan_renderer.remove_render_object(id);
}

void Renderer::update_render_object(RenderObjectId id, const glm::mat4& transform)
{
	_vulkan_renderer.update_render_object(id, transform);
}
