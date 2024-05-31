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

		FirstPersonControllerComponent()
		{
			register_siblings_request<InputComponent>(_input_components);
			register_sibling_request<TransformComponent>(&_transform_component);
		}

		void update(float delta_time) override;

	private:
		std::vector<InputComponent*> _input_components;
		TransformComponent* _transform_component = nullptr;
	};
}