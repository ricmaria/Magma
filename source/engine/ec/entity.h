#pragma once

#include "component.h"

#include <memory>
#include <unordered_map>
#include <type_traits>

namespace EC
{
	class Entity
	{
	public:

		using ComponentType = char*;

		template<typename TComponent>
		static ComponentType get_component_type() { return typeid(TComponent).name(); }

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		bool add_component(std::unique_ptr<TComponent>&& component)
		{
			const ComponentType type = get_component_type<TComponent>();

			auto[it, inserted] = type_to_component.insert({ type , std::move(component) });

			return inserted;
		}

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		bool remove_component()
		{
			const ComponentType type = get_component_type<TComponent>();

			const size_t num_erased = type_to_component.erase(type);

			return num_erased;
		}

		template<typename TComponent,
			typename = std::enable_if_t<std::is_base_of_v<Component, TComponent>> >
		Component* get_component() const
		{
			const ComponentType type = get_component_type<TComponent>();

			auto component = type_to_component.find(type);

			return (component != type_to_component.end() ? component->second.get() : nullptr);
		}

		void update(float delta_time);

	private:
		std::unordered_map<ComponentType, std::unique_ptr<Component>> type_to_component;

	};
};