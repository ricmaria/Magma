#include "gltf_mesh_render_component.h"

void EC::GltfMeshRenderComponent::set_gltf_file_path(const std::string& gltf_file_path)
{
	if (gltf_file_path == m_gltf_file_path)
	{
		return;
	}

	m_gltf_file_path = gltf_file_path;

	if (can_add_to_renderer())
	{
		remove_from_renderer();
		add_to_renderer();
	}
}

bool EC::GltfMeshRenderComponent::can_add_to_renderer()
{
	return !m_gltf_file_path.empty() && ParentType::can_add_to_renderer();
}

void EC::GltfMeshRenderComponent::add_to_renderer()
{
	m_render_object_id = m_renderer->add_gltf_mesh_render_object(m_gltf_file_path, m_transform_component->get_transform().get_matrix());
}