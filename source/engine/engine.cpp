#include <engine.h>

void MagmaEngine::init()
{
	_sdl_manager.init(1700, 900);

	_renderer.init(_sdl_manager.get_width(), _sdl_manager.get_height(), _sdl_manager.get_sdl_window());
}

void MagmaEngine::run()
{
	_sdl_manager.run([&]
		{
			if (_sdl_manager.is_window_visible())
			{
				_renderer.draw();
			}
		}
	);
}

void MagmaEngine::cleanup()
{
	_renderer.cleanup();

	_sdl_manager.cleanup();
}