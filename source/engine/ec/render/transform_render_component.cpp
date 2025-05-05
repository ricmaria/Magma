#include "transform_render_component.h"

#include "ec/transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void TransformRenderComponent::update(float delta_time)
{
	assert(m_renderer);
	assert(m_transform_component);

	if (m_render_object_id == Renderer::invalid_render_object_id)
	{
		return;
	}

	m_renderer->update_render_object(m_render_object_id, m_transform_component->get_transform());
}

bool EC::TransformRenderComponent::can_add_to_renderer()
{
	return m_transform_component != nullptr && ParentType::can_add_to_renderer();
}