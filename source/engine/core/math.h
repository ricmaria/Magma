#pragma once

#include <glm/mat4x4.hpp>

class Transform
{
public:
	Transform(const glm::mat4x4& matrix) : m_matrix(matrix) {}
	Transform() : m_matrix(glm::mat4x4{ 1.0f }) {}

	inline const glm::mat4x4& get_matrix()
	{
		return m_matrix;
	}

	inline glm::vec3 get_x() const
	{
		return m_matrix[0];
	}

	inline void set_x(const glm::vec3& x)
	{
		m_matrix[0] = glm::vec4(x, 0.0);
	}

	inline glm::vec3 get_y() const
	{
		return m_matrix[1];
	}

	inline void set_y(const glm::vec3& y)
	{
		m_matrix[1] = glm::vec4(y, 0.0);
	}

	inline glm::vec3 get_z() const
	{
		return m_matrix[2];
	}

	inline void set_z(const glm::vec3& z)
	{
		m_matrix[2] = glm::vec4(z, 0.0);
	}

	inline glm::vec3 get_translation() const
	{
		return m_matrix[3];
	}

	inline void set_translation(const glm::vec3& position)
	{
		m_matrix[3] = glm::vec4(position, 1.0);
	}

private:
	glm::mat4x4 m_matrix;
};