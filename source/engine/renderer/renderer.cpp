#include "renderer.h"

#include "vulkan_renderer.h"

void Renderer::init(uint32_t width, uint32_t height, SDL_Window* window)
{
	m_vulkan_renderer.init(width, height, window);
}

void Renderer::process_sdl_event(const SDL_Event* sdl_event)
{
	m_vulkan_renderer.process_sdl_event(sdl_event);
}

void Renderer::draw()
{
	m_vulkan_renderer.draw();
}

void Renderer::cleanup()
{
	m_vulkan_renderer.cleanup();
}

void Renderer::set_camera_transform(const Transform& transform)
{
	m_vulkan_renderer.set_camera_transform(transform);
}

void Renderer::set_directional_light_direction(const glm::vec3& direction)
{
	m_vulkan_renderer.set_directional_light_direction(direction);
}

void Renderer::set_directional_light_color(const glm::vec4& color)
{
	m_vulkan_renderer.set_directional_light_color(color);
}

Renderer::RenderObjectId Renderer::add_predefined_mesh_render_object(const std::string& mesh_name, const Transform& transform)
{
	return m_vulkan_renderer.add_render_object_predefined_mesh(mesh_name, transform);
}

Renderer::RenderObjectId Renderer::add_gltf_mesh_render_object(const std::string& gltf_file_path, const Transform& transform)
{
	return m_vulkan_renderer.add_render_object_gltf_mesh(gltf_file_path, transform);
}

void Renderer::remove_render_object(RenderObjectId id)
{
	m_vulkan_renderer.remove_render_object(id);
}

void Renderer::update_render_object(RenderObjectId id, const Transform& transform)
{
	m_vulkan_renderer.update_render_object(id, transform);
}
