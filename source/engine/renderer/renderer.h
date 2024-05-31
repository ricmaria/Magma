#pragma once

#include <memory>
#include "core/reflectable.h"
#include "vulkan_renderer.h"
#include <glm/vec3.hpp>

class Renderer : public Reflectable
{
public:
	using ParentType = Reflectable;

	std::vector<TypeId> get_types() const override
	{
		static std::vector<TypeId> type_ids = register_type_and_get_types<Renderer, ParentType>();
		return type_ids;
	}

	void init(uint32_t width, uint32_t height, struct SDL_Window* window);
	void draw();
	void cleanup();

	void set_camera_position(const glm::vec3& position);

private:
	VulkanRenderer _vulkan_renderer;
};