#include "camera_component.h"

#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void CameraComponent::update(float delta_time)
{
	const auto& position = _transform_component->get_position();
	
	_renderer->set_camera_position(position);

	const auto& x = _transform_component->get_x();
	const auto& y = _transform_component->get_y();
	const auto& z = _transform_component->get_z();

	_renderer->set_camera_axes(x, y, z);
}