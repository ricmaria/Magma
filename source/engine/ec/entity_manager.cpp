#include "entity_manager.h"

using namespace EC;

Entity* EntityManager::create_entity()
{
	Entity* entity = new Entity(_injector);

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

