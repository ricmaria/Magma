#pragma once

#include "component.h"
#include <glm/vec3.hpp>

namespace EC
{
	class TransformComponent : public Component
	{
	public:
		TransformComponent()
		{
			register_my_type<decltype(*this)>();
		}

		inline const glm::vec3& get_position() const
		{
			return _position;
		}

		inline glm::vec3& get_position()
		{
			return _position;
		}

		inline void set_position(const glm::vec3& position)
		{
			_position = position;
		}

		inline const glm::vec3& get_forward() const
		{
			return _forward;
		}

		inline const glm::vec3& get_right() const
		{
			return _right;
		}


		inline const glm::vec3& get_up() const
		{
			return _up;
		}

	private:
		glm::vec3 _position = { 0, 0, 0 };

		glm::vec3 _forward = { 0, 0, 1 };
		glm::vec3 _right = { -1, 0, 0 };
		glm::vec3 _up = { 0, -1, 0 };
	};
}