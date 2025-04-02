#pragma once

#include "component.h"
#include "core/math.h"

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

		Transform& get_transform() { return m_transform; }

	private:
		Transform m_transform = glm::mat4x4{ 1.0f };
	};
}