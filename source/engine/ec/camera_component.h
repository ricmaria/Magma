#pragma once

#include "component.h"

class Renderer;

namespace EC
{
	class TransformComponent;

	class CameraComponent : public Component
	{
	public:

		CameraComponent()
		{
			register_my_type<decltype(*this)>();

			register_sibling_request<TransformComponent>(&_transform_component);
		}

		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency dependency = Dependency::make(&CameraComponent::_renderer);
			static auto dependencies = register_dependencies<Component>(dependency);
			return dependencies;
		}

		void update(float delta_time) override;
	
	private:
		TransformComponent* _transform_component = nullptr;

		Renderer* _renderer = nullptr;
	};
}