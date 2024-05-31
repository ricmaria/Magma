#pragma once

#include "input_component.h"

namespace EC
{
	class KeyboardInputComponent : public InputComponent
	{
	public:

		using ParentType = InputComponent;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<KeyboardInputComponent, ParentType>();
			return type_ids;
		}

		void on_being_added() override;

		float get_up() const override;
		float get_down() const override;
		float get_left() const override;
		float get_right() const override;
		float get_high() const override;
		float get_low() const override;
		float get_run() const override;

		virtual ~KeyboardInputComponent() override {};

	private:
		const uint8_t* _sdl_keyboard_state_array = nullptr;
	};
}