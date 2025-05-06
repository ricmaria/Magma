#pragma once

#include "render_component.h"

namespace EC
{
	class DirectionaLightRenderComponent : public RenderComponent
	{
		using ParentType = RenderComponent;
	public:
		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<DirectionaLightRenderComponent, ParentType>();
			return type_ids;
		}

		inline void set_direction(const glm::vec3& direction) { m_direction = direction; }
		inline void set_color(const glm::vec4& color) { m_color = color; }

		void update(float delta_time) override;

	protected:
		bool is_on_renderer() override { return true; };
		void add_to_renderer() override;

		glm::vec3 m_direction = { 0.0f, 1.0f, 0.5f };
		glm::vec4 m_color = glm::vec4{ 1.0f };
	};
}