#include "renderer/renderer.h"
#include "core/magma_sdl_manager.h"
#include "core/time_manager.h"
#include "ec/entity_manager.h"
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

	template<typename TService>
	TService* GetService() { return nullptr; }

	template<>
	EC::EntityManager* GetService<EC::EntityManager>()
	{
		auto* res = _entity_manager.get();
		assert(res != nullptr);
		return res;
	}

private:

	bool update();

	SDLManager _sdl_manager;
	VulkanRenderer _renderer;
	TimeManager _time_manager;
	std::unique_ptr<EC::EntityManager> _entity_manager;
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