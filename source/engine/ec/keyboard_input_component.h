#pragma once

#include "input_component.h"

namespace EC
{
	class KeyboardInputComponent : public InputComponent
	{
	public:

		KeyboardInputComponent()
		{
			register_my_type<decltype(*this)>();
		}

		void on_being_added() override;

		float get_up() const override;
		float get_down() const override;
		float get_left() const override;
		float get_right() const override;

		virtual ~KeyboardInputComponent() override {};

	private:
		const uint8_t* _sdl_keyboard_state_array = nullptr;
	};
}