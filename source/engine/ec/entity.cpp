#include "entity.h"

using namespace EC;

void Entity::update(float delta_time)
{
	for (auto& component : _components)
	{
		component->update(delta_time);
	}
}

std::unique_ptr<Component> Entity::remove_component(Component* component_arg)
{
	auto it = std::find_if(_components.begin(), _components.end(),
		[&component_arg](std::unique_ptr<Component>& component_lambda)
		{
			return component_lambda.get() == component_arg;
		});

	return remove_component(it);
}

std::unique_ptr<Component> Entity::remove_component(const std::vector<std::unique_ptr<Component>>::iterator& it)
{
	if (it == _components.end())
	{
		return nullptr;
	}

	Component* removed_component = (*it).get();

	for (auto& component : _components)
	{
		removed_component->on_sibling_component_removed(component.get());

		component->on_sibling_component_removed(removed_component);
	}

	removed_component->on_being_removed();

	std::unique_ptr<Component> component_unique = std::move(*it);

	_components.erase(it);

	return component_unique;
	
}