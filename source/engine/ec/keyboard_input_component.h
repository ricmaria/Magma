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

		void on_being_added(const std::vector<std::unique_ptr<Component>>& siblings) override;

		bool is_up_pressed() const override;
		bool is_down_pressed() const override;
		bool is_left_pressed() const override;
		bool is_right_pressed() const override;

		virtual ~KeyboardInputComponent() override {};

	private:
		const uint8_t* _sdl_keyboard_state_array = nullptr;
	};
}