#include "entity.h"

using namespace EC;

void Entity::update(float delta_time)
{
	for (auto& component : m_components)
	{
		component->update(delta_time);
	}
}

std::unique_ptr<Component> Entity::remove_component(Component* component_arg)
{
	auto it = std::find_if(m_components.begin(), m_components.end(),
		[&component_arg](std::unique_ptr<Component>& component_lambda)
		{
			return component_lambda.get() == component_arg;
		});

	return remove_component(it);
}

std::unique_ptr<Component> Entity::remove_component(const std::vector<std::unique_ptr<Component>>::iterator& it)
{
	if (it == m_components.end())
	{
		return nullptr;
	}

	Component* removed_component = (*it).get();

	for (auto& other_component : m_components)
	{
		Injector::eject_all(*removed_component);

		// We must specify the template argument for eject_one as the component is both Injectee (first) and Reflectable
		// If we pass it as a Reflectable, downcasting the pointer value will return the wrong value
		
		Injector::eject_all_types_of_injected<Component>(*other_component, removed_component);

		removed_component->on_sibling_component_removed(other_component.get());

		other_component->on_sibling_component_removed(removed_component);
	}

	removed_component->on_being_removed();

	std::unique_ptr<Component> component_unique = std::move(*it);

	m_components.erase(it);

	return component_unique;
	
}