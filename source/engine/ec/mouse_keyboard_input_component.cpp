#include "mouse_keyboard_input_component.h"

#include <SDL.h>

using namespace EC;

void MouseKeyboardInputComponent::on_being_added()
{
	m_sdl_keyboard_state_array = SDL_GetKeyboardState(nullptr);
}

void MouseKeyboardInputComponent::update(float delta_time)
{
	int32_t x, y;
	uint32_t buttons_bitmask = SDL_GetRelativeMouseState(&x, &y);

	m_mouse_delta.x = static_cast<float>(x);
	m_mouse_delta.y = static_cast<float>(y);
	m_action_1 = (SDL_BUTTON(1) & buttons_bitmask) > 0 ? 1.0f : 0.0f;
	m_action_2 = (SDL_BUTTON(2) & buttons_bitmask) > 0 ? 1.0f : 0.0f;
}

float MouseKeyboardInputComponent::MouseKeyboardInputComponent::get_forward() const
{
	return std::max(m_sdl_keyboard_state_array[SDL_SCANCODE_W], m_sdl_keyboard_state_array[SDL_SCANCODE_UP])
			- std::max(m_sdl_keyboard_state_array[SDL_SCANCODE_S], m_sdl_keyboard_state_array[SDL_SCANCODE_DOWN]);
}

float MouseKeyboardInputComponent::get_strafe() const
{
	return std::max(m_sdl_keyboard_state_array[SDL_SCANCODE_D], m_sdl_keyboard_state_array[SDL_SCANCODE_RIGHT])
			- std::max(m_sdl_keyboard_state_array[SDL_SCANCODE_A], m_sdl_keyboard_state_array[SDL_SCANCODE_LEFT]);
}

float MouseKeyboardInputComponent::MouseKeyboardInputComponent::get_fly() const
{
	return std::max(m_sdl_keyboard_state_array[SDL_SCANCODE_E], m_sdl_keyboard_state_array[SDL_SCANCODE_PAGEUP])
			- std::max(m_sdl_keyboard_state_array[SDL_SCANCODE_Q], m_sdl_keyboard_state_array[SDL_SCANCODE_PAGEDOWN]);
}

float MouseKeyboardInputComponent::get_run() const
{
	return std::max(m_sdl_keyboard_state_array[SDL_SCANCODE_LSHIFT], m_sdl_keyboard_state_array[SDL_SCANCODE_RSHIFT]);
}

float MouseKeyboardInputComponent::get_action_1() const
{
	return m_action_1;
}

float MouseKeyboardInputComponent::get_action_2() const
{
	return m_action_2;
}

float MouseKeyboardInputComponent::get_yaw() const
{
	return m_mouse_delta.x;
}

float MouseKeyboardInputComponent::get_pitch() const
{
	return m_mouse_delta.y;
}