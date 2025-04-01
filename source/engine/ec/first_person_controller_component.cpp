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

	const glm::vec3 global_up_vec{ 0.0f, 1.0f, 0.0f };

	glm::vec3 forward_vec = - m_transform_component->get_z();
	glm::vec3 right_vec = m_transform_component->get_x();

	right_vec = glm::rotate(right_vec, yaw_angle, global_up_vec);
	forward_vec = glm::rotate(forward_vec, yaw_angle, global_up_vec);

	glm::vec3 up_vec = glm::cross(right_vec, forward_vec);
	
	right_vec = glm::cross(forward_vec, up_vec);
	right_vec = glm::normalize(right_vec);

	glm::vec3 rotated_forward_vec = glm::rotate(forward_vec, pitch_angle, right_vec);
	rotated_forward_vec = glm::normalize(rotated_forward_vec);
	
	if (rotated_forward_vec.x * forward_vec.x > 0.0f && rotated_forward_vec.z * forward_vec.z > 0.0f)
	{
		forward_vec = rotated_forward_vec;
	}

	up_vec = glm::cross(right_vec, forward_vec);
	up_vec = glm::normalize(up_vec);

	m_transform_component->set_z(- forward_vec);
	m_transform_component->set_x(right_vec);
	m_transform_component->set_y(up_vec);

	{
		static bool test_rotation = false;

		if (test_rotation)
		{
			glm::vec3 forward_db = - m_transform_component->get_z();
			glm::vec3 right_db = m_transform_component->get_x();

			const float rotation_speed = 1.0f;
			float rotation_angle = rotation_speed * delta_time;

			forward_db = glm::rotate(forward_db, rotation_angle, global_up_vec);
			forward_db = glm::normalize(forward_db);

			right_db = glm::rotate(right_db, rotation_angle, global_up_vec);

			glm::vec3 up_db = glm::cross(right_db, forward_db);
			up_db = glm::normalize(up_db);

			right_db = glm::cross(forward_db, up_db);
			right_db = glm::normalize(right_db);

			m_transform_component->set_z(- forward_db);
			m_transform_component->set_x(right_db);
			m_transform_component->set_y(up_db);
		}		
	}

	const float default_speed = 20.0f;
	const float run_speed = 100.0f;

	float speed = std::max(default_speed, run_speed * run_input);

	glm::vec3 position = m_transform_component->get_translation();

	position += (- m_transform_component->get_z()) * forward_input * speed * delta_time;
	position += m_transform_component->get_x() * strafe_input * speed * delta_time;
	position += m_transform_component->get_y() * fly_input * speed * delta_time;

	m_transform_component->set_translation(position);
}