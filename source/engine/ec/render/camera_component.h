#pragma once

#include "ec/component.h"

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
			static Dependency renderer = Dependency::make(&CameraComponent::m_renderer);
			static Dependency transform = Dependency::make(&CameraComponent::m_transform_component);
			static auto dependencies = append_dependencies(ParentType::get_dependencies(), { renderer, transform });
			
			return dependencies;
		}
	
	private:
		TransformComponent* m_transform_component = nullptr;

		Renderer* m_renderer = nullptr;
	};
}