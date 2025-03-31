#include "mesh_render_component.h"
#include "mesh_render_component.h"

#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void EC::MeshRenderComponent::on_sibling_component_added(Component* sibling)
{
	Component::on_sibling_component_added(sibling);

	if (sibling->is_of_type(extract_type<TransformComponent>()))
	{
		if (! m_mesh_name.empty())
		{
			add_mesh_to_renderer();
		}
	}	
}

void MeshRenderComponent::on_being_removed()
{
	m_renderer->remove_render_object(m_render_object_id);

	m_render_object_id = Renderer::invalid_render_object_id;
}

void MeshRenderComponent::update(float delta_time)
{
	m_renderer->update_render_object(m_render_object_id, m_transform_component->get_transform());
}

void EC::MeshRenderComponent::set_mesh_name(const std::string& mesh_name)
{
	if (mesh_name == m_mesh_name)
	{
		return;
	}

	if (m_render_object_id != Renderer::invalid_render_object_id)
	{
		m_renderer->remove_render_object(m_render_object_id);
	}

	m_mesh_name = mesh_name;
			
	add_mesh_to_renderer();
}

void EC::MeshRenderComponent::add_mesh_to_renderer()
{
	m_render_object_id = m_renderer->add_render_object(m_mesh_name, m_transform_component->get_transform());
}
