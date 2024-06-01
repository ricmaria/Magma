#pragma once

#include "reflectable.h"
#include <SDL.h>

class Logger: public Reflectable
{
public:
	template<typename... Args>
	void log(const Args&... args)
	{
		SDL_Log(args...);
	}
};