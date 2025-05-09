#pragma once

#include "ec/component.h"
#include "renderer/renderer.h"

namespace EC
{
	class RenderComponent : public Component
	{
	public:
		using ParentType = Component;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<RenderComponent, ParentType>();
			return type_ids;
		}

		void on_sibling_component_added(Component* sibling) override;
		void on_sibling_component_removed(Component* sibling) override;
		void on_being_removed() override;

	protected:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency renderer = Dependency::make(&RenderComponent::m_renderer);
			static auto dependencies = append_dependencies(ParentType::get_dependencies(), { renderer });

			return dependencies;
		}

		virtual bool is_on_renderer() = 0;

		virtual bool can_be_on_renderer();

		virtual void add_to_renderer() {};

		virtual void remove_from_renderer() {};

		void reset_on_render();

		Renderer* m_renderer = nullptr;
	};
}