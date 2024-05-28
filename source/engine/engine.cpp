#include <engine.h>

void MagmaEngine::init(Features features)
{
	_service_locator.register_service<Renderer>(&_renderer);
	_service_locator.register_service<TimeManager>(&_time_manager);

	std::bitset<4> bit_set(static_cast<size_t>(features) << 1);	// Shift left as the bitset starts at position 0, which is the value for None

	if (bit_set.test(static_cast<size_t>(Features::EC)))
	{
		_entity_manager = std::make_unique<EC::EntityManager>();
		log("Entity Manager initialized");

		_service_locator.register_service(_entity_manager.get());
	}

	if (bit_set.test(static_cast<size_t>(Features::Test)))
	{
		log("Test initialized");
	}

	_sdl_manager.init(1700, 900);

	_renderer.init(_sdl_manager.get_width(), _sdl_manager.get_height(), _sdl_manager.get_sdl_window());

	_time_manager.init();

	if (_entity_manager)
	{
		_entity_manager->init(&_service_locator);
	}

	_last_time_update = _time_manager.get_elapsed_time();
}

void MagmaEngine::run()
{
	_sdl_manager.run(Delegate<bool>(this, &MagmaEngine::update));
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

	return true;
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