#pragma once

#include "render_object_component.h"

#include <string>

namespace EC
{
	class PredefinedMeshRenderComponent : public RenderObjectComponent
	{
		using ParentType = RenderObjectComponent;
	public:
		const std::string& get_mesh_name() const { return m_mesh_name; }
		void set_mesh_name(const std::string& mesh_name);

	protected:
		bool can_be_on_renderer() override;
		void add_to_renderer() override;
	private:

		std::string m_mesh_name;
	};
}