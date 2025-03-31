#pragma once

#include "component.h"
#include "renderer/renderer.h"

#include <string>

namespace EC
{
	class TransformComponent;

	class MeshRenderComponent : public Component
	{
	public:

		using ParentType = Component;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<MeshRenderComponent, ParentType>();
			return type_ids;
		}

		void on_sibling_component_added(Component* sibling) override;
		void on_being_removed() override;
		void update(float delta_time) override;

		const std::string& get_mesh_name() const { return m_mesh_name; }
		void set_mesh_name(const std::string& mesh_name);

	protected:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency renderer = Dependency::make(&MeshRenderComponent::m_renderer);
			static Dependency transform = Dependency::make(&MeshRenderComponent::m_transform_component);
			static auto dependencies = append_dependencies(ParentType::get_dependencies(), { renderer, transform });

			return dependencies;
		}

	private:

		void add_mesh_to_renderer();

		TransformComponent* m_transform_component = nullptr;
		Renderer* m_renderer = nullptr;

		std::string m_mesh_name;

		Renderer::RenderObjectId m_render_object_id = Renderer::invalid_render_object_id;
	};
}