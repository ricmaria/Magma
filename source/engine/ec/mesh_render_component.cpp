#include "mesh_render_component.h"

#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void MeshRenderComponent::on_being_added()
{
	_renderer->add_render_object(_mesh_name.c_str(), _material_name.c_str(), glm::mat4{ 1.0f });
}