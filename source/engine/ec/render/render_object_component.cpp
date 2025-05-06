#include "render_object_component.h"

#include "ec/transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void RenderObjectComponent::update(float delta_time)
{
	assert(m_renderer);
	assert(m_transform_component);

	if (m_render_object_id == Renderer::invalid_render_object_id)
	{
		return;
	}

	m_renderer->update_render_object(m_render_object_id, m_transform_component->get_transform());
}

bool RenderObjectComponent::is_on_renderer()
{
	return m_render_object_id != Renderer::invalid_render_object_id;
}

bool RenderObjectComponent::can_be_on_renderer()
{
	return m_transform_component != nullptr && ParentType::can_be_on_renderer();
}

void RenderObjectComponent::remove_from_renderer()
{
	m_renderer->remove_render_object(m_render_object_id);

	m_render_object_id = Renderer::invalid_render_object_id;
}