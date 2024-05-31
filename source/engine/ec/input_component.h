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

		virtual float get_up() const = 0;
		virtual float get_down() const = 0;
		virtual float get_left() const = 0;
		virtual float get_right() const = 0;
		virtual float get_high() const = 0;
		virtual float get_low() const = 0;
		virtual float get_run() const = 0;

		virtual ~InputComponent() override {};
	};
}