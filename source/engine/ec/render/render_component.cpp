#include "render_component.h"

#include "ec/transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void EC::RenderComponent::on_sibling_component_added(Component* sibling)
{
	ParentType::on_sibling_component_added(sibling);

	if (can_add_to_renderer())
	{
		add_to_renderer();
	}
}

void EC::RenderComponent::on_sibling_component_removed(Component* sibling)
{
	remove_from_renderer();

	ParentType::on_sibling_component_removed(sibling);
}

void RenderComponent::on_being_removed()
{
	remove_from_renderer();

	ParentType::on_being_removed();
}

bool EC::RenderComponent::can_add_to_renderer()
{
	return m_renderer != nullptr;
}

void EC::RenderComponent::remove_from_renderer()
{
	if (m_render_object_id == Renderer::invalid_render_object_id)
	{
		return;
	}

	m_renderer->remove_render_object(m_render_object_id);

	m_render_object_id = Renderer::invalid_render_object_id;
}