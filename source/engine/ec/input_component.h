#pragma once

#include "component.h"

namespace EC
{
	class InputComponent : public Component
	{
	public:
		virtual ~InputComponent() override {};
		virtual void init() override {};
		virtual void update(float delta_time) override {};
		virtual void exit() override {};
	};
}