#include "entity_manager.h"

using namespace EC;

Entity* EntityManager::create_entity()
{
	Entity* entity = new Entity;

	entity->_service_locator = _service_locator;

	_entities.push_back(std::unique_ptr<Entity>(entity));

	return _entities.back().get();
}

void EntityManager::update(float delta_time)
{
	for (auto& entity : _entities)
	{
		entity->update(delta_time);
	}
}

