#pragma once

#include "input_component.h"
#include <glm/vec2.hpp>

namespace EC
{
	class MouseKeyboardInputComponent : public InputComponent
	{
	public:

		using ParentType = InputComponent;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<MouseKeyboardInputComponent, ParentType>();
			return type_ids;
		}

		void on_being_added() override;

		void update(float delta_time) override;

		float get_forward() const override;
		float get_strafe() const override;
		float get_fly() const override;
		float get_run() const override;
		float get_action_1() const override;
		float get_action_2() const override;

		virtual float get_pitch() const override;
		virtual float get_yaw() const override;

		virtual ~MouseKeyboardInputComponent() override {};

	private:
		const uint8_t* _sdl_keyboard_state_array = nullptr;
		float _action_1 = 0.0f;
		float _action_2 = 0.0f;
		glm::vec2 _mouse_delta = { 0.0f,0.0f };
	};
}