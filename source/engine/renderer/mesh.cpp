#include "mesh.h"

void MeshNode::draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
	glm::mat4 nodeMatrix = topMatrix * worldTransform;

	for (auto& surface : mesh->surfaces)
	{
		RenderObject def;
		def.indexCount = surface.count;
		def.firstIndex = surface.startIndex;
		def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
		def.material = &surface.material->data;
		def.bounds = surface.bounds;
		def.transform = nodeMatrix;
		def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

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
	Node::draw(topMatrix, ctx);
}