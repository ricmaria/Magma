#include "camera_component.h"

#include "core/service_locator.h"
#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void CameraComponent::on_being_added()
{
	_renderer = _service_locator->get_service<Renderer>();
}

void CameraComponent::update(float delta_time)
{
	glm::vec3 position = _transform_component->get_position();

	_renderer->set_camera_position(position);
}