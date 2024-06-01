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

		void update(float delta_time) override;

	protected:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency renderer = Dependency::make(&CameraComponent::_renderer);
			static Dependency transform = Dependency::make(&CameraComponent::_transform_component);
			static auto dependencies = register_and_get_dependencies<ParentType>({ renderer, transform });
			
			return dependencies;
		}
	
	private:
		TransformComponent* _transform_component = nullptr;

		Renderer* _renderer = nullptr;
	};
}