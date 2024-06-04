#pragma once

#include "component.h"

namespace EC
{
	class InputComponent : public Component
	{
	public:

		using ParentType = Component;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<InputComponent, ParentType>();
			return type_ids;
		}

		virtual float get_forward() const = 0;
		virtual float get_lateral() const = 0;
		virtual float get_vertical() const = 0;
		virtual float get_run() const = 0;
		virtual float get_action_1() const = 0;
		virtual float get_action_2() const = 0;

		virtual float get_pitch() const = 0;
		virtual float get_yaw() const = 0;

		virtual ~InputComponent() override {};
	};
}