#include "mesh_render_component.h"
#include "mesh_render_component.h"

#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void MeshRenderComponent::update(float delta_time)
{
	assert(m_renderer);
	assert(m_transform_component);

	if (m_render_object_id == Renderer::invalid_render_object_id)
	{
		return;
	}

	m_renderer->update_render_object(m_render_object_id, m_transform_component->get_transform());
}

bool EC::MeshRenderComponent::can_add_to_renderer()
{
	return m_transform_component != nullptr && ParentType::can_add_to_renderer();
}

void EC::PredefinedMeshRenderComponent::set_mesh_name(const std::string& mesh_name)
{
	if (mesh_name == m_mesh_name)
	{
		return;
	}

	m_mesh_name = mesh_name;

	if (can_add_to_renderer())
	{
		remove_from_renderer();
		add_to_renderer();
	}
}

bool EC::PredefinedMeshRenderComponent::can_add_to_renderer()
{
	return !m_mesh_name.empty() && ParentType::can_add_to_renderer();
}

void EC::PredefinedMeshRenderComponent::add_to_renderer()
{
	m_render_object_id = m_renderer->add_predefined_mesh_render_object(m_mesh_name, m_transform_component->get_transform().get_matrix());
}

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
