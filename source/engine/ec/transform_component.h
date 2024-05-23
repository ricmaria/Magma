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

		inline glm::vec3 get_position() const
		{
			return _position;
		}

		inline float get_position_x() const
		{
			return _position.x;
		}

		inline float get_position_y() const
		{
			return _position.y;
		}

		inline float get_position_z() const
		{
			return _position.z;
		}

		inline void set_position(const glm::vec3& position)
		{
			_position = position;
		}

		inline void set_position_x(float x)
		{
			_position.x = x;
		}

		inline void set_position_y(float y)
		{
			_position.y = y;
		}

		inline void set_position_z(float z)
		{
			_position.z = z;
		}

	private:
		glm::vec3 _position = { 0, 0, 0 };
	};
}