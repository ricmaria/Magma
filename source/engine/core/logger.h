#pragma once

#include <SDL.h>
#include <glm/glm.hpp>

class Logger
{
public:
	template<typename... Args>
	void log(const Args&... args)
	{
		SDL_Log(args...);
	}

	void log_vec3(const char * text, const glm::vec3& vector)
	{
		SDL_Log("%s x %f y %f z %f", text, vector.x, vector.y, vector.z);
	}
};