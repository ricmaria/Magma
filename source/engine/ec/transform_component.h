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

		inline const glm::mat4x4& get_transform()
		{
			return m_transform;
		}

		inline glm::vec3 get_x() const
		{
			return m_transform[0];
		}

		inline void set_x(const glm::vec3& x)
		{
			m_transform[0] = glm::vec4(x, 0.0);
		}

		inline glm::vec3 get_y() const
		{
			return m_transform[1];
		}

		inline void set_y(const glm::vec3& y)
		{
			m_transform[1] = glm::vec4(y, 0.0);
		}

		inline glm::vec3 get_z() const
		{
			return m_transform[2];
		}

		inline void set_z(const glm::vec3& z)
		{
			m_transform[2] = glm::vec4(z, 0.0);
		}		

		inline glm::vec3 get_translation() const
		{
			return m_transform[3];
		}

		inline void set_translation(const glm::vec3& position)
		{
			m_transform[3] = glm::vec4(position, 1.0);
		}

	private:
		glm::mat4x4 m_transform = glm::mat4x4{ 1.0f };
	};
}