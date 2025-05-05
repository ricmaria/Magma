#pragma once

#include "render_component.h"
#include "ec/transform_component.h"

namespace EC
{
	class TransformRenderComponent : public RenderComponent
	{
		using ParentType = RenderComponent;
	public:
		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<TransformRenderComponent, ParentType>();
			return type_ids;
		}

		void update(float delta_time) override;

	protected:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency transform = Dependency::make(&TransformRenderComponent::m_transform_component);
			static auto dependencies = append_dependencies(ParentType::get_dependencies(), { transform });

			return dependencies;
		}

		bool can_add_to_renderer() override;

		TransformComponent* m_transform_component = nullptr;
	};
}