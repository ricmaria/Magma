#pragma once

#include "component.h"

namespace EC
{
	class CameraComponent : public Component
	{
	public:

		CameraComponent()
		{
			register_my_type<decltype(*this)>();
		}

		void update(float delta_time) override;
	};
}