#pragma once

#include <memory>
#include <functional>

#include "core/reflectable.h"
#include "core/injectee.h"

namespace EC
{
	class Component: public Injectee, public Reflectable // Injectee must be the first to allow downcast
	{
		friend class Entity;

	public:
		virtual ~Component() {};

		using ParentType = Reflectable;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<Component, ParentType>();
			return type_ids;
		}

	protected:
		virtual void update(float delta_time) {};

		virtual void on_being_added() {};

		virtual void on_being_removed() {};

		virtual void on_sibling_component_added(Component* sibling) {};

		virtual void on_sibling_component_removed(Component* sibling) {};
	};
};