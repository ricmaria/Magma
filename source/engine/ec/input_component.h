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

		virtual float get_up() const = 0;
		virtual float get_down() const = 0;
		virtual float get_left() const = 0;
		virtual float get_right() const = 0;

		virtual ~InputComponent() override {};
	};
}