#include "directional_light_component.h"
#include "renderer/renderer.h"

using namespace EC;

void DirectionaLightRenderComponent::add_to_renderer()
{
	m_renderer->set_directional_light_direction(m_direction);
	m_renderer->set_directional_light_color(m_color);
}

void DirectionaLightRenderComponent::update(float delta_time)
{
	assert(m_renderer);

	m_renderer->set_directional_light_direction(m_direction);
	m_renderer->set_directional_light_color(m_color);
}