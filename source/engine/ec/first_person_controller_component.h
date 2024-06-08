#pragma once

#include "component.h"
#include <glm/vec3.hpp>

namespace EC
{
	class InputComponent;
	class TransformComponent;

	class FirstPersonControllerComponent : public Component
	{
	public:

		using ParentType = Component;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<FirstPersonControllerComponent, ParentType>();
			return type_ids;
		}

		void update(float delta_time) override;

		bool get_invert_mouse_y() { return _invert_mouse_y; }
		bool set_invert_mouse_y(bool value) { _invert_mouse_y = value; }

	protected:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency transform = Dependency::make(&FirstPersonControllerComponent::_transform_component);
			static Dependency input = Dependency::make(&FirstPersonControllerComponent::_input_components);
			static auto dependencies = append_dependencies(ParentType::get_dependencies(), { transform, input });
			
			return dependencies;
		}

	private:
		std::vector<InputComponent*> _input_components;
		TransformComponent* _transform_component = nullptr;
		bool _invert_mouse_y = false;
	};
}