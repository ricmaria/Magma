#pragma once

#include "component.h"

namespace EC
{
	class InputComponent;
	class TransformComponent;

	class CameraComponent : public Component
	{
	public:

		CameraComponent()
		{
			register_my_type<decltype(*this)>();

			request_siblings<InputComponent>(_input_components);
			request_sibling<TransformComponent>(&_transform_component);
		}

		void update(float delta_time) override;

		virtual ~CameraComponent() override {};
	private:
		std::vector<InputComponent*> _input_components;
		TransformComponent* _transform_component = nullptr;
	};
}