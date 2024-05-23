#include "keyboard_input_component.h"

#include <SDL.h>

using namespace EC;

void KeyboardInputComponent::on_being_added(const std::vector<std::unique_ptr<Component>>& siblings)
{
	_sdl_keyboard_state_array = SDL_GetKeyboardState(nullptr);
}

bool KeyboardInputComponent::KeyboardInputComponent::is_up_pressed() const
{
	return _sdl_keyboard_state_array[SDLK_w] | _sdl_keyboard_state_array[SDLK_UP];
}

bool KeyboardInputComponent::is_down_pressed() const
{
	return _sdl_keyboard_state_array[SDLK_s] | _sdl_keyboard_state_array[SDLK_DOWN];
}

bool KeyboardInputComponent::is_left_pressed() const
{
	return _sdl_keyboard_state_array[SDLK_a] | _sdl_keyboard_state_array[SDLK_LEFT];
}

bool KeyboardInputComponent::is_right_pressed() const
{
	return _sdl_keyboard_state_array[SDLK_d] | _sdl_keyboard_state_array[SDLK_RIGHT];
}