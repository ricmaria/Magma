
#include "magma_sdl_manager.h"

#include <SDL.h>
#include <SDL_vulkan.h>

void SDLManager::init(uint32_t width, uint32_t height)
{
	_width = width;
	_height = height;

	// We initialize SDL and create a window with it. 
	SDL_Init(SDL_INIT_VIDEO);

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

	_window = SDL_CreateWindow(
		"Magma Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_width,
		_height,
		window_flags
	);
}

void SDLManager::run(Delegate<bool> on_loop)
{
	SDL_Event e;
	bool bQuit = false;

	//main loop
	while (!bQuit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//close the window when user alt-f4s or clicks the X button			
			if (e.type == SDL_QUIT)
			{
				bQuit = true;
			}
			else if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_SPACE)
				{
				}
			}
		}

		bQuit |= ! on_loop();
	}
}

void SDLManager::cleanup()
{
	SDL_DestroyWindow(_window);
}

bool SDLManager::is_window_visible() const
{
	return ! ((SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED));
}

void SDLManager::log(const char* text)
{
	SDL_Log(text);
}