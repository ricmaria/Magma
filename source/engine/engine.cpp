#include <engine.h>

void MagmaEngine::init()
{
	_renderer.init();
}

void MagmaEngine::run()
{
	_renderer.run();
}

void MagmaEngine::cleanup()
{
	_renderer.cleanup();
}