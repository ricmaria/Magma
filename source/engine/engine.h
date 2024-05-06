#include "renderer/renderer.h"

#pragma once

class MagmaEngine
{
public:
	void init();
	void run();
	void cleanup();

private:
	VulkanRenderer _renderer;
};