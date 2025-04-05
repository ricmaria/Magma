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
		void on_sibling_component_removed(Component* sibling) override;
		void on_being_removed() override;
		void update(float delta_time) override;

	protected:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency renderer = Dependency::make(&MeshRenderComponent::m_renderer);
			static Dependency transform = Dependency::make(&MeshRenderComponent::m_transform_component);
			static auto dependencies = append_dependencies(ParentType::get_dependencies(), { renderer, transform });

			return dependencies;
		}

		virtual bool can_add_mesh_to_renderer() = 0;
		virtual void add_mesh_to_renderer() = 0;

		void remove_mesh_from_renderer();

		TransformComponent* m_transform_component = nullptr;
		Renderer* m_renderer = nullptr;

		Renderer::RenderObjectId m_render_object_id = Renderer::invalid_render_object_id;
	};

	class PredefinedMeshRenderComponent : public MeshRenderComponent
	{
		using Super = MeshRenderComponent;
	public:
		const std::string& get_mesh_name() const { return m_mesh_name; }
		void set_mesh_name(const std::string& mesh_name);

	protected:
		bool can_add_mesh_to_renderer() override;
		void add_mesh_to_renderer() override;
	private:

		std::string m_mesh_name;
	};
}