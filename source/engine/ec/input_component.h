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

		virtual bool is_up_pressed() const = 0;
		virtual bool is_down_pressed() const = 0;
		virtual bool is_left_pressed() const = 0;
		virtual bool is_right_pressed() const = 0;

		virtual ~InputComponent() override {};
	};
}