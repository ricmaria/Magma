#pragma once

#include "render_component.h"

#include <string>

namespace EC
{
	class TransformComponent;

	class MeshRenderComponent : public RenderComponent
	{
		using ParentType = RenderComponent;
	public:
		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<MeshRenderComponent, ParentType>();
			return type_ids;
		}

		void update(float delta_time) override;

	protected:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency transform = Dependency::make(&MeshRenderComponent::m_transform_component);
			static auto dependencies = append_dependencies(ParentType::get_dependencies(), { transform });

			return dependencies;
		}

		bool can_add_to_renderer() override;

		TransformComponent* m_transform_component = nullptr;
	};

	class PredefinedMeshRenderComponent : public MeshRenderComponent
	{
		using ParentType = MeshRenderComponent;
	public:
		const std::string& get_mesh_name() const { return m_mesh_name; }
		void set_mesh_name(const std::string& mesh_name);

	protected:
		bool can_add_to_renderer() override;
		void add_to_renderer() override;
	private:

		std::string m_mesh_name;
	};

	class GltfMeshRenderComponent : public MeshRenderComponent
	{
		using ParentType = MeshRenderComponent;
	public:
		const std::string& get_gltf_file_path() const { return m_gltf_file_path; }
		void set_gltf_file_path(const std::string& gltf_file_path);

	protected:
		bool can_add_to_renderer() override;
		void add_to_renderer() override;
	private:

		std::string m_gltf_file_path;
	};
}