#pragma once

#include "component.h"

namespace EC
{
	class InputComponent : public Component
	{
	public:

		InputComponent()
		{
			register_my_type<decltype(*this)>();
		}

		virtual ~InputComponent() override {};
		virtual void init() override {};
		virtual void update(float delta_time) override {};
		virtual void exit() override {};
	};
}