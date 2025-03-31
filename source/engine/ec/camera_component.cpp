#include "camera_component.h"

#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void CameraComponent::update(float delta_time)
{
	const auto& position = _transform_component->get_position();
	
	_renderer->set_camera_position(position);

	const auto& forward = _transform_component->get_forward();
	const auto& right = _transform_component->get_right();
	const auto& up = _transform_component->get_up();

	_renderer->set_camera_axes(right, up, -forward);
}