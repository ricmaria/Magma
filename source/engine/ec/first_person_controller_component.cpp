#include "first_person_controller_component.h"

#include <algorithm>

#include "input_component.h"
#include "transform_component.h"
#include <glm/gtx/rotate_vector.hpp>

using namespace EC;

void FirstPersonControllerComponent::update(float delta_time)
{
	assert(_input_components.size() > 0);
	assert(_transform_component);

	float forward_input = 0.0f;
	float lateral_input = 0.0f;
	float vertical_input = 0.0f;
	float run_input = 0.0f;
	float pitch_input = 0.0f;
	float yaw_input = 0.0f;

	auto update_input_val = [](float& val, float new_val)
		{
			val = std::abs(new_val) > std::abs(val) ? new_val : val;
		};

	for (auto* input_component: _input_components)
	{
		update_input_val(forward_input, input_component->get_forward());
		update_input_val(lateral_input, input_component->get_lateral());
		update_input_val(vertical_input, input_component->get_vertical());
		update_input_val(run_input, input_component->get_run());
		update_input_val(pitch_input, input_component->get_pitch());
		update_input_val(yaw_input, input_component->get_yaw());
	}

	glm::vec3 forward_vec = _transform_component->get_forward();
	glm::vec3 right_vec = _transform_component->get_right();

	const glm::vec3 global_up_vec{ 0.0f, 1.0f, 0.0f };

	right_vec = glm::rotate(right_vec, yaw_input, global_up_vec);
	forward_vec = glm::rotate(forward_vec, yaw_input, global_up_vec);
	
	glm::vec3 up_vec = glm::cross(right_vec, forward_vec);
	right_vec = glm::cross(forward_vec, up_vec);

	forward_vec = glm::rotate(forward_vec, pitch_input, right_vec);

	up_vec = glm::cross(right_vec, forward_vec);

	forward_vec = glm::normalize(forward_vec);
	if (forward_vec.y > 0.99f)
	{
		forward_vec.y = 0.99f;
		forward_vec = glm::normalize(forward_vec);
	}

	right_vec = glm::normalize(right_vec);
	up_vec = glm::normalize(up_vec);
	
	//_transform_component->set_forward(forward_vec);
	//_transform_component->set_right(right_vec);
	//_transform_component->set_up(up_vec);	

	const float default_speed = 1.0f;
	const float run_speed = 5.0f;

	float speed = std::max(default_speed, run_speed * run_input);

	glm::vec3 position = _transform_component->get_position();

	
	position += _transform_component->get_forward() * forward_input * speed * delta_time;
	position += _transform_component->get_right() * lateral_input * speed * delta_time;
	position += _transform_component->get_up() * vertical_input * speed * delta_time;

	_transform_component->set_position(position);
}