#include "camera_component.h"

#include "transform_component.h"

using namespace EC;

void CameraComponent::update(float delta_time)
{
	_transform_component->get_position();
}