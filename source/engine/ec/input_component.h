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
	};
}