#pragma once

#include "input_component.h"

namespace EC
{
	class KeyboardInputComponent : public Component
	{
	public:

		KeyboardInputComponent()
		{
			register_my_type<decltype(*this)>();
		}

		virtual ~KeyboardInputComponent() override {};
	};
}