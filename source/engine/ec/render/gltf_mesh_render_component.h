#pragma once

#include "render_object_component.h"

#include <string>

namespace EC
{
	class GltfMeshRenderComponent : public RenderObjectComponent
	{
		using ParentType = RenderObjectComponent;
	public:
		const std::string& get_gltf_file_path() const { return m_gltf_file_path; }
		void set_gltf_file_path(const std::string& gltf_file_path);

	protected:
		bool can_be_on_renderer() override;
		void add_to_renderer() override;
	private:

		std::string m_gltf_file_path;
	};
}