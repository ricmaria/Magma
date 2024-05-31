#pragma once

#include "component.h"

class Renderer;

namespace EC
{
	class TransformComponent;

	class CameraComponent : public Component
	{
	public:

		using ParentType = Component;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<CameraComponent, ParentType>();
			return type_ids;
		}

		CameraComponent()
		{
			register_sibling_request<TransformComponent>(&_transform_component);
		}

		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency dependency = Dependency::make(&CameraComponent::_renderer);
			static auto dependencies = register_dependencies<ParentType>(dependency);
			return dependencies;
		}

		void update(float delta_time) override;
	
	private:
		TransformComponent* _transform_component = nullptr;

		Renderer* _renderer = nullptr;
	};
}