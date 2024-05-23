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

			request_sibling<InputComponent>((void**) &_input_component);
			request_sibling<TransformComponent>((void**) &_transform_component);
		}

		void update(float delta_time) override;

		virtual ~CameraComponent() override {};
	private:
		InputComponent* _input_component = nullptr;
		TransformComponent* _transform_component = nullptr;
	};
}