#pragma once

#include "entity.h"
#include "component.h"
#include <vector>

class ServiceLocator;

namespace EC
{
	class EntityManager
	{
	public:
		inline void init(ServiceLocator* service_locator)
		{
			_service_locator = service_locator;
		}

		Entity* create_entity();

		void update(float delta_time);

	private:
		std::vector<std::unique_ptr<Entity>> _entities;

		ServiceLocator* _service_locator;
	};
}