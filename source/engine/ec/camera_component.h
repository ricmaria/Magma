#pragma once

#include "component.h"

class Renderer;

namespace EC
{
	class TransformComponent;

	class CameraComponent : public Component
	{
	public:

		using ParentType = Component;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<CameraComponent, ParentType>();
			return type_ids;
		}

	protected:
		std::vector<SiblingRequest> get_sibling_requests() const override
		{
			static SiblingRequest transform = SiblingRequest::make(&CameraComponent::_transform_component);
			static std::vector<SiblingRequest> requests = add_and_get_siblings_requests<ParentType>({ transform });
			return requests;
		}

	public:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency dependency = Dependency::make(&CameraComponent::_renderer);
			static auto dependencies = register_and_get_dependencies<ParentType>({ dependency });
			return dependencies;
		}

		void update(float delta_time) override;
	
	private:
		TransformComponent* _transform_component = nullptr;

		Renderer* _renderer = nullptr;
	};
}