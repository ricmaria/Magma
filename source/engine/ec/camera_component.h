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

		void on_being_added() override;

		void update(float delta_time) override;
	
	private:
		TransformComponent* _transform_component = nullptr;

		Renderer* _renderer = nullptr;
	};
}