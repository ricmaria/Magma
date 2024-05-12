#include "renderer/renderer.h"
#include "core/magma_sdl_manager.h"

#pragma once

class MagmaEngine
{
public:
	void init();
	void run();
	void cleanup();

private:
	SDLManager _sdl_manager;
	VulkanRenderer _renderer;
};