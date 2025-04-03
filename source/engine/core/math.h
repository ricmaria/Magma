#pragma once

#include <numbers>

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/common.hpp>

namespace Math
{
	inline float constrain_angle_360(float angle_deg)
	{
		angle_deg = std::fmod(angle_deg, 360);
		if (angle_deg < 0)
		{
			angle_deg += 360;
		}
		return angle_deg;
	}

	inline float constrain_angle_2pi(float angle_rad)
	{
		angle_rad = std::fmod(angle_rad, 2.0f * std::numbers::pi);
		if (angle_rad < 0)
		{
			angle_rad += 2.0f * std::numbers::pi;
		}
		return angle_rad;
	}

	inline glm::vec3 constrain_angle_2pi(const glm::vec3& angle_rad_vec)
	{
		glm::vec3 res = { constrain_angle_2pi(angle_rad_vec.x), constrain_angle_2pi(angle_rad_vec.y), constrain_angle_2pi(angle_rad_vec.z) };

		return res;
	}
}

class Transform
{
public:
	Transform()
	{
		m_position = glm::vec3(0.0f, 0.0f, 0.0f);
		m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
		m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
	}

	Transform(glm::mat4x4 matrix)
	{
		glm::mat3x3 axes_matrix;

		axes_matrix[0] = glm::normalize(matrix[0]);
		axes_matrix[1] = glm::normalize(matrix[1]);
		axes_matrix[2] = glm::normalize(matrix[2]);

		glm::quat rotation_quat = glm::quat_cast(axes_matrix);

		m_rotation = glm::eulerAngles(rotation_quat);

		m_position = matrix[3];

		m_scale = { glm::length(matrix[0]), glm::length(matrix[1]), glm::length(matrix[2]) };
	}

	inline glm::mat4x4 get_matrix() const
	{
		glm::quat rotation_quat = glm::quat(m_rotation);

		glm::mat4x4 rotation_matrix = glm::mat4_cast(rotation_quat);

		glm::mat4x4 translation_matrix = glm::translate(glm::mat4x4{ 1.0f }, m_position);

		glm::mat4x4 scale_matrix = glm::scale(glm::mat4x4{ 1.0f }, m_scale);

		glm::mat4x4 matrix = scale_matrix * translation_matrix * rotation_matrix;

		return matrix;
	}

	inline glm::vec3 get_position() const
	{
		return m_position;
	}

	inline void set_position(const glm::vec3& position)
	{
		m_position = position;
	}

	inline glm::vec3 get_rotation() const
	{
		return m_rotation;
	}

	inline void set_rotation(const glm::vec3& rotation)
	{
		m_rotation = Math::constrain_angle_2pi(rotation);
	}

	inline glm::vec3 get_scale() const
	{
		return m_scale;
	}

	inline void set_scale(const glm::vec3& scale)
	{
		m_scale = scale;
	}

	inline glm::vec3 get_axis_x() const
	{
		glm::quat rotation_quat = glm::quat(m_rotation);
		glm::vec3 axis_x = rotation_quat * glm::vec3(1.0f, 0.0f, 0.0f);

		return axis_x;
	}

	inline glm::vec3 get_axis_y() const
	{
		glm::quat rotation_quat = glm::quat(m_rotation);
		glm::vec3 axis_y = rotation_quat * glm::vec3(0.0f, 1.0f, 0.0f);

		return axis_y;
	}

	inline glm::vec3 get_axis_z() const
	{
		glm::quat rotation_quat = glm::quat(m_rotation);
		glm::vec3 axis_z = rotation_quat * glm::vec3(0.0f, 0.0f, 1.0f);

		return axis_z;
	}

private:
	glm::vec3 m_position;
	glm::vec3 m_rotation;
	glm::vec3 m_scale;
};