#include "mouse_keyboard_input_component.h"

#include <SDL.h>

using namespace EC;

void MouseKeyboardInputComponent::on_being_added()
{
	_sdl_keyboard_state_array = SDL_GetKeyboardState(nullptr);
}

void MouseKeyboardInputComponent::update(float delta_time)
{
	int32_t x, y;
	uint32_t buttons_bitmask = SDL_GetRelativeMouseState(&x, &y);

	_mouse_delta.x = static_cast<float>(x);
	_mouse_delta.y = static_cast<float>(y);
	_action_1 = (SDL_BUTTON(1) & buttons_bitmask) > 0 ? 1.0f : 0.0f;
	_action_2 = (SDL_BUTTON(2) & buttons_bitmask) > 0 ? 1.0f : 0.0f;
}

float MouseKeyboardInputComponent::MouseKeyboardInputComponent::get_forward() const
{
	return std::max(_sdl_keyboard_state_array[SDL_SCANCODE_W], _sdl_keyboard_state_array[SDL_SCANCODE_UP])
			- std::max(_sdl_keyboard_state_array[SDL_SCANCODE_S], _sdl_keyboard_state_array[SDL_SCANCODE_DOWN]);
}

float MouseKeyboardInputComponent::get_strafe() const
{
	return std::max(_sdl_keyboard_state_array[SDL_SCANCODE_D], _sdl_keyboard_state_array[SDL_SCANCODE_RIGHT])
			- std::max(_sdl_keyboard_state_array[SDL_SCANCODE_A], _sdl_keyboard_state_array[SDL_SCANCODE_LEFT]);
}

float MouseKeyboardInputComponent::MouseKeyboardInputComponent::get_fly() const
{
	return std::max(_sdl_keyboard_state_array[SDL_SCANCODE_E], _sdl_keyboard_state_array[SDL_SCANCODE_PAGEUP])
			- std::max(_sdl_keyboard_state_array[SDL_SCANCODE_Q], _sdl_keyboard_state_array[SDL_SCANCODE_PAGEDOWN]);
}

float MouseKeyboardInputComponent::get_run() const
{
	return std::max(_sdl_keyboard_state_array[SDL_SCANCODE_LSHIFT], _sdl_keyboard_state_array[SDL_SCANCODE_RSHIFT]);
}

float MouseKeyboardInputComponent::get_action_1() const
{
	return _action_1;
}

float MouseKeyboardInputComponent::get_action_2() const
{
	return _action_2;
}

float MouseKeyboardInputComponent::get_yaw() const
{
	return _mouse_delta.x;
}

float MouseKeyboardInputComponent::get_pitch() const
{
	return _mouse_delta.y;
}