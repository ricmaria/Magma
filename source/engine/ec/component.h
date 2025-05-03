#pragma once

#include <memory>
#include <functional>

#include "core/injectee.h"
#include "core/logger.h"
#include "core/reflectable.h"


namespace EC
{
	class Component: public Injectee, public Reflectable // Injectee must be the first to allow downcast
	{
		friend class Entity;

	public:
		virtual ~Component() {};

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<Component, Reflectable>();
			return type_ids;
		}

	protected:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency logger = Dependency::make(&Component::m_logger);
			static auto dependencies = append_dependencies(Injectee::get_dependencies(), { logger });

			return dependencies;
		}

		virtual void update(float delta_time) {};

		virtual void on_being_added() {};

		virtual void on_being_removed() {};

		virtual void on_sibling_component_added(Component* sibling) {};

		virtual void on_sibling_component_removed(Component* sibling) {};

		Logger* m_logger;
	};
};