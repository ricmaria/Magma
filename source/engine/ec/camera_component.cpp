#include "camera_component.h"

#include "input_component.h"
#include "transform_component.h"

using namespace EC;

void CameraComponent::update(float delta_time)
{
	assert(_input_component);
	assert(_transform_component);

	const float speed = 1.0f;
}