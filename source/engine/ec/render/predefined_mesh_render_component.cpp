#include "predefined_mesh_render_component.h"

void EC::PredefinedMeshRenderComponent::set_mesh_name(const std::string& mesh_name)
{
	if (mesh_name == m_mesh_name)
	{
		return;
	}

	m_mesh_name = mesh_name;

	reset_on_render();
}

bool EC::PredefinedMeshRenderComponent::can_be_on_renderer()
{
	return !m_mesh_name.empty() && ParentType::can_be_on_renderer();
}

void EC::PredefinedMeshRenderComponent::add_to_renderer()
{
	m_render_object_id = m_renderer->add_predefined_mesh_render_object(m_mesh_name, m_transform_component->get_transform().get_matrix());
}