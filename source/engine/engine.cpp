#include "engine.h"

#include <thread>

void MagmaEngine::init(Features features)
{
	std::bitset<4> bit_set(static_cast<size_t>(features) << 1);	// Shift left as the bitset starts at position 0, which is the value for None

	if (bit_set.test(static_cast<size_t>(Features::EC)))
	{
		_entity_manager = std::make_unique<EC::EntityManager>();
		
		_logger.log("Entity Manager initialized");
	}

	if (bit_set.test(static_cast<size_t>(Features::Test)))
	{
		_logger.log("Test initialized");
	}

	_sdl_manager.init(1700, 900);

	_injector.register_one_type(&_logger);

	_renderer.init(_sdl_manager.get_width(), _sdl_manager.get_height(), _sdl_manager.get_sdl_window());
	_injector.register_one_type(&_renderer);

	_time_manager.init();
	_injector.register_one_type(&_time_manager);

	if (_entity_manager)
	{
		_entity_manager->init(&_injector);
	}

	_last_time_update = _time_manager.get_elapsed_time();
}

void MagmaEngine::run()
{
	_logger.log("MagmaEngine loop started.");

	auto process_sdl_event = std::bind(&MagmaEngine::process_sdl_event, this, std::placeholders::_1);
	auto update = std::bind(&MagmaEngine::update, this);

	_sdl_manager.run(process_sdl_event, update);
}

void MagmaEngine::process_sdl_event(const SDL_Event* sdl_event)
{
	_renderer.process_sdl_event(sdl_event);
}

bool MagmaEngine::update()
{
	float elapsed_time = _time_manager.get_elapsed_time();
	float delta_time = elapsed_time - _last_time_update;
	_last_time_update = elapsed_time;

	if (_entity_manager)
	{
		_entity_manager->update(delta_time);
	}

	if (_sdl_manager.is_window_visible())
	{
		_renderer.draw();
	}
	else
	{
		// throttle the speed to avoid the endless spinning
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return true;
}

void MagmaEngine::cleanup()
{
	_renderer.cleanup();

	_sdl_manager.cleanup();
}