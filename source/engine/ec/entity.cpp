#include "entity.h"

using namespace EC;

void Entity::update(float delta_time)
{
	for (auto& type_and_component : type_to_component)
	{
		type_and_component.second->update(delta_time);
	}
}