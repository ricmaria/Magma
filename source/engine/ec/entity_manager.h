#pragma once

#include "entity.h"
#include "component.h"
#include <vector>

class Injector;

namespace EC
{
	class EntityManager
	{
	public:
		inline void init(Injector* injector)
		{
			m_injector = injector;
		}

		Entity* create_entity();

		void update(float delta_time);

	private:
		std::vector<std::unique_ptr<Entity>> m_entities;

		Injector* m_injector;
	};
}