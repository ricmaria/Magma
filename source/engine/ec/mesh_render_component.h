#pragma once

#include "component.h"

#include <string>

class Renderer;

namespace EC
{
	class TransformComponent;

	class MeshRenderComponent : public Component
	{
	public:

		using ParentType = Component;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<MeshRenderComponent, ParentType>();
			return type_ids;
		}

		void on_being_added() override;

		const std::string& get_mesh_name() { return _mesh_name; }
		void set_mesh_name(const std::string& mesh_name) { _mesh_name = mesh_name; }

		const std::string& get_material_name() { return _material_name; }
		void set_material_name(const std::string& material_name) { _material_name = material_name; }

	protected:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency renderer = Dependency::make(&MeshRenderComponent::_renderer);
			static Dependency transform = Dependency::make(&MeshRenderComponent::_transform_component);
			static auto dependencies = append_dependencies(ParentType::get_dependencies(), { renderer, transform });

			return dependencies;
		}

	private:
		TransformComponent* _transform_component = nullptr;
		Renderer* _renderer = nullptr;

		std::string _mesh_name;
		std::string _material_name;
	};
}