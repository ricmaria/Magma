#include "mesh_render_component.h"

#include "transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void MeshRenderComponent::on_being_added()
{
	_render_object_id = _renderer->add_render_object(_mesh_name, _temp_transform);
}

void MeshRenderComponent::on_being_removed()
{
	_renderer->remove_render_object(_render_object_id);

	_render_object_id = Renderer::invalid_render_object_id;
}