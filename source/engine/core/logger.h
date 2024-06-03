#pragma once

#include <SDL.h>

class Logger
{
public:
	template<typename... Args>
	void log(const Args&... args)
	{
		SDL_Log(args...);
	}
};