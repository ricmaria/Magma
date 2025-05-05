#pragma once

#include "transform_render_component.h"

#include <string>

namespace EC
{
	class PredefinedMeshRenderComponent : public TransformRenderComponent
	{
		using ParentType = TransformRenderComponent;
	public:
		const std::string& get_mesh_name() const { return m_mesh_name; }
		void set_mesh_name(const std::string& mesh_name);

	protected:
		bool can_add_to_renderer() override;
		void add_to_renderer() override;
	private:

		std::string m_mesh_name;
	};
}