#pragma once

#include "component.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace EC
{
	class TransformComponent : public Component
	{
	public:
		using ParentType = Component;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<TransformComponent, ParentType>();
			return type_ids;
		}

		inline const glm::vec3& get_position() const
		{
			return m_position;
		}

		inline void set_position(const glm::vec3& position)
		{
			m_position = position;
			m_dirty = true;
		}

		inline const glm::vec3& get_forward() const
		{
			return m_forward;
		}

		inline void set_forward(const glm::vec3& forward)
		{
			m_forward = forward;
			m_dirty = true;
		}

		inline const glm::vec3& get_right() const
		{
			return m_right;
		}

		inline void set_right(const glm::vec3& right)
		{
			m_right = right;
			m_dirty = true;
		}

		inline const glm::vec3& get_up() const
		{
			return m_up;
		}

		inline void set_up(const glm::vec3& up)
		{
			m_up = up;
			m_dirty = true;
		}

		inline const glm::mat4x4& get_transform()
		{
			if (m_dirty)
			{
				m_transform[0] = glm::vec4(m_right, 0.0f);
				m_transform[1] = glm::vec4(m_up, 0.0f);
				m_transform[2] = glm::vec4(- m_forward, 0.0f);
				m_transform[3] = glm::vec4(m_position, 1.0f);
				m_dirty = false;
			}

			return m_transform;
		}

	private:
		glm::vec3 m_position = { 0, 0, 0 };

		glm::vec3 m_right = { 1, 0, 0 };
		glm::vec3 m_up = { 0, 1, 0 };
		glm::vec3 m_forward = { 0, 0, -1 };

		glm::mat4x4 m_transform;

		bool m_dirty = true;
	};
}