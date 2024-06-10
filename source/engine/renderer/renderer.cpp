#include "renderer.h"

#include "vulkan_renderer.h"

void Renderer::init(uint32_t width, uint32_t height, SDL_Window* window)
{
	_vulkan_renderer.init(width, height, window);
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

void Renderer::set_camera_view(const glm::vec3& forward, const glm::vec3& right, const glm::vec3& up)
{
	_vulkan_renderer.set_camera_view(forward, right, up);
}

RenderObjectId Renderer::add_render_object(const std::string& mesh_name, const std::string& material_name, glm::mat4 transform)
{
	return _vulkan_renderer.add_render_object(mesh_name, material_name, transform);
}

void Renderer::remove_render_object(RenderObjectId id)
{
	_vulkan_renderer.remove_render_object(id);
}