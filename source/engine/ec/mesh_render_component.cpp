#include "mesh_render_component.h"
#include "mesh_render_component.h"

#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void EC::MeshRenderComponent::on_sibling_component_added(Component* sibling)
{
	ParentType::on_sibling_component_added(sibling);

	add_mesh_to_renderer();
}

void EC::MeshRenderComponent::on_sibling_component_removed(Component* sibling)
{
	ParentType::on_sibling_component_removed(sibling);

	remove_mesh_from_renderer();
}

void MeshRenderComponent::on_being_removed()
{
	ParentType::on_being_removed();

	remove_mesh_from_renderer();
}

void MeshRenderComponent::update(float delta_time)
{
	assert(m_renderer);
	assert(m_transform_component);

	if (m_render_object_id == Renderer::invalid_render_object_id)
	{
		return;
	}

	m_renderer->update_render_object(m_render_object_id, m_transform_component->get_transform().get_matrix());
}

void EC::MeshRenderComponent::set_mesh_name(const std::string& mesh_name)
{
	if (mesh_name == m_mesh_name)
	{
		return;
	}

	m_mesh_name = mesh_name;

	add_mesh_to_renderer();
}

void EC::MeshRenderComponent::add_mesh_to_renderer()
{
	remove_mesh_from_renderer();

	if (m_mesh_name.empty())
	{
		return;
	}

	if (m_transform_component == nullptr)
	{
		return;
	}

	if (m_renderer == nullptr)
	{
		return;
	}

	m_render_object_id = m_renderer->add_render_object(m_mesh_name, m_transform_component->get_transform().get_matrix());
}

void EC::MeshRenderComponent::remove_mesh_from_renderer()
{
	if (m_render_object_id == Renderer::invalid_render_object_id)
	{
		return;
	}

	m_renderer->remove_render_object(m_render_object_id);

	m_render_object_id = Renderer::invalid_render_object_id;
}
