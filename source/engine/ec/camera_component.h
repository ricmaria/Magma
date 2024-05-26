#pragma once

#include "component.h"

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

		void update(float delta_time) override;
	
	private:
		TransformComponent* _transform_component = nullptr;
	};
}