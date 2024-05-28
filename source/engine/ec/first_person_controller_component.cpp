#include "first_person_controller_component.h"

#include "input_component.h"
#include "transform_component.h"

#include <SDL.h>

using namespace EC;

void FirstPersonControllerComponent::update(float delta_time)
{
	assert(_input_components.size() > 0);
	assert(_transform_component);

	float up = 0.0f;
	float down = 0.0f;
	float left = 0.0f;
	float right = 0.0f;
	float high = 0.0f;
	float low = 0.0f;
	float run = 0.0f;

	for (auto* input_component: _input_components)
	{
		up = std::max(up, input_component->get_up());
		down = std::max(down, input_component->get_down());
		left = std::max(left, input_component->get_left());
		right = std::max(right, input_component->get_right());
		high = std::max(right, input_component->get_high());
		low = std::max(right, input_component->get_low());
		run = std::max(run, input_component->get_run());
	}

	const float default_speed = 1.0f;
	const float run_speed = 5.0f;

	float speed = std::max(default_speed, run_speed * run);

	glm::vec3 position = _transform_component->get_position();
	
	glm::vec3 debug_position = position;
	
	position += _transform_component->get_forward() * up * speed * delta_time;
	position -= _transform_component->get_forward() * down * speed * delta_time;
	position += _transform_component->get_right() * right * speed * delta_time;
	position -= _transform_component->get_right() * left * speed * delta_time;
	position += _transform_component->get_up() * high * speed * delta_time;
	position -= _transform_component->get_up() * low * speed * delta_time;

	_transform_component->set_position(position);
}