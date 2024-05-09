#pragma once

#include "component.h"

#include <memory>
#include <vector>
#include <type_traits>

namespace EC
{
	class Entity
	{
	public:

	
		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		TComponent* add_component(std::unique_ptr<TComponent>&& component)
		{
			components.push_back(component);

			// TODO: implement notifications to components

			return components.back().get();
		}

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		std::unique_ptr<Component> remove_component()
		{
			auto it = std::find(components.begin(), components.end(),
				[](std::unique_ptr<Component>& component)
				{
					return component->is_of_type<TComponent>();
				});

			return remove_component(it);
		}

		std::unique_ptr<Component> remove_component(Component* component_arg);

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		Component* get_component() const
		{
			auto it = std::find_if(components.begin(), components.end(),
				[](std::unique_ptr<Component>& component)
				{
					return component->is_of_type<TComponent>();
				});

			if (it != components.end())
			{
				return it->get();
			}

			return nullptr;
		}

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		std::vector<Component*> get_components() const
		{
			std::vector<Component*> result;

			for (auto& component : components)
			{
				if (component->is_of_type<TComponent>())
				{
					result.push_back(component.get());
				}
			}

			return result;
		}

		void update(float delta_time);

	private:
		std::vector<std::unique_ptr<Component>> components;

		std::unique_ptr<Component> remove_component(const std::vector<std::unique_ptr<Component>>::iterator& it);

	};
};