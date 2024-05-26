#include "first_person_controller_component.h"

#include "input_component.h"
#include "transform_component.h"

using namespace EC;

void FirstPersonControllerComponent::update(float delta_time)
{
	assert(_input_components.size() > 0);
	assert(_transform_component);

	float up = 0.0f;
	float down = 0.0f;
	float left = 0.0f;
	float right = 0.0f;

	for (auto* input_component: _input_components)
	{
		up = std::max(up, input_component->get_up());
		down = std::max(down, input_component->get_down());
		left = std::max(left, input_component->get_left());
		right = std::max(right, input_component->get_right());
	}

	const float speed = 1.0f;

	glm::vec3 position = _transform_component->get_position();
	position += _transform_component->get_forward() * up * speed * delta_time;
	position -= _transform_component->get_forward() * down * delta_time;
	position += _transform_component->get_right() * right * delta_time;
	position -= _transform_component->get_right() * left * delta_time;

	_transform_component->set_position(position);
}