#include "mesh.h"

void MeshNode::add_to_render_context(const glm::mat4& topMatrix, RenderContext& ctx)
{
	glm::mat4 nodeMatrix = topMatrix * worldTransform;

	for (auto& surface : mesh->surfaces)
	{
		GpuRenderObject def;
		def.indexCount = surface.count;
		def.firstIndex = surface.start_index;
		def.indexBuffer = mesh->mesh_buffers.indexBuffer.buffer;
		def.material = &surface.material->data;
		def.bounds = surface.bounds;
		def.transform = nodeMatrix;
		def.vertexBufferAddress = mesh->mesh_buffers.vertexBufferAddress;

		if (surface.material->data.passType == MaterialPass::Transparent)
		{
			ctx.TransparentSurfaces.push_back(def);
		}
		else
		{
			ctx.OpaqueSurfaces.push_back(def);
		}
	}

	// recurse down
	Node::add_to_render_context(topMatrix, ctx);
}