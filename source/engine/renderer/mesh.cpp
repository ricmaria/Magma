#include "mesh.h"

void MeshNode::add_to_render_context(const glm::mat4& topMatrix, RenderContext& ctx)
{
	glm::mat4 nodeMatrix = topMatrix * world_transform;

	for (auto& surface : mesh->surfaces)
	{
		GpuRenderObject def;
		def.index_count = surface.count;
		def.first_index = surface.start_index;
		def.index_buffer = mesh->mesh_buffers.index_buffer.buffer;
		def.material = &surface.material->data;
		def.bounds = surface.bounds;
		def.transform = nodeMatrix;
		def.vertex_buffer_address = mesh->mesh_buffers.vertex_buffer_address;

		if (surface.material->data.pass_type == MaterialPassType::Transparent)
		{
			ctx.transparent_surfaces.push_back(def);
		}
		else
		{
			ctx.opaque_surfaces.push_back(def);
		}
	}

	// recurse down
	Node::add_to_render_context(topMatrix, ctx);
}