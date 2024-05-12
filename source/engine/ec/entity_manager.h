#pragma once

#include "entity.h"
#include "component.h"
#include <vector>

namespace EC
{
	class EntityManager
	{
	public:
		Entity* create_entity();

	private:
		std::vector<std::unique_ptr<Entity>> _entities;
	};

}