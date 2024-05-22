#include <engine.h>

void MagmaEngine::init(Features features)
{
	_sdl_manager.init(1700, 900);

	_renderer.init(_sdl_manager.get_width(), _sdl_manager.get_height(), _sdl_manager.get_sdl_window());

	if (features == Features::None)
	{
		return;
	}

	std::bitset<4> bit_set(static_cast<size_t>(features));

	if (bit_set.test(static_cast<size_t>(Features::EC) - 1))
	{
		_entity_manager = std::make_unique<EC::EntityManager>();
		log("Entity Manager initialized");
	}

	if (bit_set.test(static_cast<size_t>(Features::Test) - 1))
	{
		log("Test initialized");
	}
}

void MagmaEngine::run()
{
	_sdl_manager.run([&]
		{
			if (_sdl_manager.is_window_visible())
			{
				_renderer.draw();
			}

			return true;
		}
	);
}

void MagmaEngine::cleanup()
{
	_renderer.cleanup();

	_sdl_manager.cleanup();
}

void MagmaEngine::log(const char* text)
{
	_sdl_manager.log(text);
}