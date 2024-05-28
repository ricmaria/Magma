#include "renderer/renderer.h"
#include "core/magma_sdl_manager.h"
#include "core/time_manager.h"
#include "ec/entity_manager.h"
#include "core/service_locator.h"
#include <bitset>

#pragma once

class MagmaEngine
{
public:
	enum class Features : char
	{
		None = 0,
		EC = 1,
		Test = 2
	};

	void init(Features features = Features::None);
	void run();
	void cleanup();

	void log(const char* text);

	inline const ServiceLocator& get_service_locator() const { return _service_locator; }

private:

	bool update();

	SDLManager _sdl_manager;
	Renderer _renderer;
	TimeManager _time_manager;
	std::unique_ptr<EC::EntityManager> _entity_manager;
	ServiceRegister _service_locator;

	float _last_time_update;
};

inline MagmaEngine::Features operator|(MagmaEngine::Features a, MagmaEngine::Features b)
{
	return static_cast<MagmaEngine::Features>(static_cast<char>(a) | static_cast<char>(b));
};

inline MagmaEngine::Features operator&(MagmaEngine::Features a, MagmaEngine::Features b)
{
	return static_cast<MagmaEngine::Features>(static_cast<char>(a) & static_cast<char>(b));
}