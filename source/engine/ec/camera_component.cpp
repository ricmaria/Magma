#include "camera_component.h"

#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void CameraComponent::update(float delta_time)
{
	const auto& transform = m_transform_component->get_transform();
	
	m_renderer->set_camera_transform(transform);
}