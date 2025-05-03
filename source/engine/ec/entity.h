#pragma once

#include "component.h"
#include "core/injector.h"

#include <memory>
#include <vector>
#include <type_traits>

namespace EC
{
	class Entity
	{
		friend class EntityManager;
	
	public:
		
		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		static std::unique_ptr<TComponent> create_component()
		{
			return std::make_unique<TComponent>();
		}

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		TComponent* add_component()
		{
			return static_cast<TComponent*>(add_component(create_component<TComponent>()));
		}

		Component* add_component(std::unique_ptr<Component>&& component)
		{
			Component* new_component = component.get();

			m_external_injector->inject(*new_component);

			new_component->on_being_added();

			for (auto& other_component_unique : m_components)
			{
				auto other_component = other_component_unique.get();

				// We must specify the template argument for inject_one as the component is both Injectee (first) and Reflectable
				// If we pass it as a Reflectable, downcasting the pointer value will return the wrong value

				Injector::inject_all_types_of_injected<Component>(*other_component, new_component);

				Injector::inject_all_types_of_injected<Component>(*new_component, other_component);

				new_component->on_sibling_component_added(other_component);

				other_component->on_sibling_component_added(new_component);
			}

			m_components.push_back(std::move(component));

			return m_components.back().get();
		}

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		std::unique_ptr<Component> remove_component()
		{
			auto it = std::find_if(m_components.begin(), m_components.end(),
				[](const std::unique_ptr<Component>& component)
				{
					return component->is_of_type<TComponent>();
				});

			return remove_component(it);
		}

		std::unique_ptr<Component> remove_component(Component* component_arg);

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		std::vector<std::unique_ptr<Component>> remove_components()
		{
			auto erase_begin = std::remove_if(m_components.begin(), m_components.end(),
				[](std::unique_ptr<Component>& component)
				{
					return component->is_of_type<TComponent>();
				});

			std::vector<std::unique_ptr<Component>> result;

			for (auto it = erase_begin; it != m_components.end(); ++it)
			{
				result.push_back(std::move(*it));
			}

			std::erase(erase_begin, m_components.end());

			return result;
		}

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		Component* get_component() const
		{
			auto it = std::find_if(m_components.begin(), m_components.end(),
				[](std::unique_ptr<Component>& component)
				{
					return component->is_of_type<TComponent>();
				});

			if (it != m_components.end())
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

			for (auto& component : m_components)
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

		Entity(Injector* injector) : m_external_injector(injector) {};

		Injector* m_external_injector = nullptr;

		std::vector<std::unique_ptr<Component>> m_components;

		std::unique_ptr<Component> remove_component(const std::vector<std::unique_ptr<Component>>::iterator& it);

	};
};