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

}