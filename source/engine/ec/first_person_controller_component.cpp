#include "first_person_controller_component.h"

#include <algorithm>

#include "input_component.h"
#include "transform_component.h"
#include <glm/gtx/rotate_vector.hpp>

using namespace EC;

void FirstPersonControllerComponent::update(float delta_time)
{
	assert(m_input_components.size() > 0);
	assert(m_transform_component);

	float forward_input = 0.0f;
	float strafe_input = 0.0f;
	float fly_input = 0.0f;
	float run_input = 0.0f;
	float pitch_input = 0.0f;
	float yaw_input = 0.0f;
	float action_1 = 0.0f;

	auto update_input_val = [](float& val, float new_val)
		{
			val = std::abs(new_val) > std::abs(val) ? new_val : val;
		};

	for (auto* input_component : m_input_components)
	{
		update_input_val(forward_input, input_component->get_forward());
		update_input_val(strafe_input, input_component->get_strafe());
		update_input_val(fly_input, input_component->get_fly());
		update_input_val(run_input, input_component->get_run());
		update_input_val(yaw_input, input_component->get_yaw());
		update_input_val(pitch_input, input_component->get_pitch());
		update_input_val(action_1, input_component->get_action_1());
	}

	const float yaw_speed = 0.5f;
	const float pitch_speed = 0.5f;

	const float yaw_angle = -yaw_input * yaw_speed * delta_time * action_1;
	const float pitch_angle = -pitch_input * pitch_speed * delta_time * action_1 * (m_invert_mouse_y ? -1.0f : 1.0f);

	auto& rotation = m_transform_component->get_transform().rotation;
	rotation.x += pitch_angle;
	rotation.y += yaw_angle;

	const float default_speed = 20.0f;
	const float run_speed = 100.0f;

	float speed = std::max(default_speed, run_speed * run_input);

	auto& position = m_transform_component->get_transform().position;

	position += (- m_transform_component->get_transform().get_axis_z()) * forward_input * speed * delta_time;
	position += m_transform_component->get_transform().get_axis_x() * strafe_input * speed * delta_time;
	position += m_transform_component->get_transform().get_axis_y() * fly_input * speed * delta_time;
}