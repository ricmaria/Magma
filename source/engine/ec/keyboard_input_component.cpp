#include "keyboard_input_component.h"

#include <SDL.h>

using namespace EC;

void KeyboardInputComponent::on_being_added()
{
	_sdl_keyboard_state_array = SDL_GetKeyboardState(nullptr);
}

float KeyboardInputComponent::KeyboardInputComponent::get_up() const
{
	return std::max(_sdl_keyboard_state_array[SDL_SCANCODE_W],_sdl_keyboard_state_array[SDL_SCANCODE_UP]);
}

float KeyboardInputComponent::get_down() const
{
	return std::max(_sdl_keyboard_state_array[SDL_SCANCODE_S], _sdl_keyboard_state_array[SDL_SCANCODE_DOWN]);
}

float KeyboardInputComponent::get_left() const
{
	return std::max(_sdl_keyboard_state_array[SDL_SCANCODE_A], _sdl_keyboard_state_array[SDL_SCANCODE_LEFT]);
}

float KeyboardInputComponent::get_right() const
{
	return std::max(_sdl_keyboard_state_array[SDL_SCANCODE_D], _sdl_keyboard_state_array[SDL_SCANCODE_RIGHT]);
}