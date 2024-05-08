#pragma once

namespace EC
{
	class Component
	{
	public:
		virtual ~Component() {};
		virtual void init() {};
		virtual void update(float delta_time) {};
		virtual void exit() {};
	};
};