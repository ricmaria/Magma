#include "entity_manager.h"

using namespace EC;

Entity* EntityManager::create_entity()
{
	Entity* entity = new Entity(m_injector);

	m_entities.push_back(std::unique_ptr<Entity>(entity));

	return m_entities.back().get();
}

void EntityManager::update(float delta_time)
{
	for (auto& entity : m_entities)
	{
		entity->update(delta_time);
	}
}

