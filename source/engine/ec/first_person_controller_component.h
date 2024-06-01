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

	protected:
		std::vector<SiblingRequest> get_sibling_requests() const override
		{
			static SiblingRequest transform = SiblingRequest::make(&FirstPersonControllerComponent::_transform_component);
			static SiblingRequest input = SiblingRequest::make(&FirstPersonControllerComponent::_input_components);
			static std::vector<SiblingRequest> requests = add_and_get_siblings_requests<ParentType>({ transform, input });
			return requests;
		}

		void update(float delta_time) override;

	private:
		std::vector<InputComponent*> _input_components;
		TransformComponent* _transform_component = nullptr;
	};
}