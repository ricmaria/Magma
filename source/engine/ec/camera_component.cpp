#include "camera_component.h"

#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void CameraComponent::update(float delta_time)
{
	glm::vec3 position = _transform_component->get_position();

	_renderer->set_camera_position(position);
}