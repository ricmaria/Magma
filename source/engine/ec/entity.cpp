#include "entity.h"

using namespace EC;

void Entity::update(float delta_time)
{
	for (auto& component : components)
	{
		component->update(delta_time);
	}
}

std::unique_ptr<Component> Entity::remove_component(Component* component_arg)
{
	auto it = std::find_if(components.begin(), components.end(),
		[&component_arg](std::unique_ptr<Component>& component_lambda)
		{
			return component_lambda.get() == component_arg;
		});

	return remove_component(it);
}

std::unique_ptr<Component> Entity::remove_component(const std::vector<std::unique_ptr<Component>>::iterator& it)
{
	if (it != components.end())
	{
		std::unique_ptr<Component> component_unique = std::move(*it);

		components.erase(it);

		// TODO: implement notifications to components

		return component_unique;
	}

	return nullptr;
}