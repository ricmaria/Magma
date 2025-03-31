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

		inline const glm::vec3& get_z() const
		{
			return m_z;
		}

		inline void set_z(const glm::vec3& z)
		{
			m_z = z;
			m_dirty = true;
		}

		inline const glm::vec3& get_x() const
		{
			return m_x;
		}

		inline void set_x(const glm::vec3& x)
		{
			m_x = x;
			m_dirty = true;
		}

		inline const glm::vec3& get_y() const
		{
			return m_y;
		}

		inline void set_y(const glm::vec3& y)
		{
			m_y = y;
			m_dirty = true;
		}

		inline const glm::mat4x4& get_transform()
		{
			if (m_dirty)
			{
				m_transform[0] = glm::vec4(m_x, 0.0f);
				m_transform[1] = glm::vec4(m_y, 0.0f);
				m_transform[2] = glm::vec4(m_z, 0.0f);
				m_transform[3] = glm::vec4(m_position, 1.0f);
				m_dirty = false;
			}

			return m_transform;
		}

	private:
		glm::vec3 m_position = { 0, 0, 0 };

		glm::vec3 m_x = { 1, 0, 0 };
		glm::vec3 m_y = { 0, 1, 0 };
		glm::vec3 m_z = { 0, 0, 1 };

		glm::mat4x4 m_transform;

		bool m_dirty = true;
	};
}