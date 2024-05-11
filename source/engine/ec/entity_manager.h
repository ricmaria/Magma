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

		template<typename TComponent>
		std::unique_ptr<TComponent> create_component()
		{
			return std::make_unique<TComponent>();
		}

	private:
		std::vector<std::unique_ptr<Entity>> _entities;
	};

}