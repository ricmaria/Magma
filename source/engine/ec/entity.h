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
		TComponent* add_component()
		{
			auto new_component_unique = std::make_unique<TComponent>();
			auto new_component = new_component_unique.get();

			_external_injector->inject(*new_component);

			new_component->on_being_added();

			for (auto& other_component_unique : _components)
			{
				auto other_component = other_component_unique.get();

				// We must specify the template argument for inject_one as the component is both Injectee (first) and Reflectable
				// If we pass it as a Reflectable, downcasting the pointer value will return the wrong value

				Injector::inject_one<Component>(*other_component, new_component);

				Injector::inject_one<Component>(*new_component, other_component);

				new_component->on_sibling_component_added(other_component);

				other_component->on_sibling_component_added(new_component);
			}

			_components.push_back(std::move(new_component_unique));

			return static_cast<TComponent*>(_components.back().get());
		}

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		std::unique_ptr<Component> remove_component()
		{
			auto it = std::find_if(_components.begin(), _components.end(),
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
			auto erase_begin = std::remove_if(_components.begin(), _components.end(),
				[](std::unique_ptr<Component>& component)
				{
					return component->is_of_type<TComponent>();
				});

			std::vector<std::unique_ptr<Component>> result;

			for (auto it = erase_begin; it != _components.end(); ++it)
			{
				result.push_back(std::move(*it));
			}

			std::erase(erase_begin, _components.end());

			return result;
		}

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		Component* get_component() const
		{
			auto it = std::find_if(_components.begin(), _components.end(),
				[](std::unique_ptr<Component>& component)
				{
					return component->is_of_type<TComponent>();
				});

			if (it != _components.end())
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

			for (auto& component : _components)
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

		Entity(Injector* injector) : _external_injector(injector) {};

		Injector* _external_injector = nullptr;

		std::vector<std::unique_ptr<Component>> _components;

		std::unique_ptr<Component> remove_component(const std::vector<std::unique_ptr<Component>>::iterator& it);

	};
};