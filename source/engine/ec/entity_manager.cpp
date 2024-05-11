#include "entity_manager.h"

using namespace EC;

Entity* EntityManager::create_entity()
{
	Entity* entity = new Entity;

	_entities.push_back(std::unique_ptr<Entity>(entity));

	return _entities.back().get();
}

