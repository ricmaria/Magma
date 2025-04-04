#include "gltf_mesh.h"

void GltfMesh::add_to_render_context(const glm::mat4& topMatrix, RenderContext& ctx)
{
	// create renderables from the scenenodes
	for (auto& top_node : m_top_nodes)
	{
		top_node->add_to_render_context(topMatrix, ctx);
	}
}

void GltfMesh::clear_all()
{
	m_descriptor_pool.destroy_pools(m_device);
	m_buffer_allocator.destroy_buffer(m_material_data_buffer);

	for (auto& [k, v] : m_meshes)
	{
		m_buffer_allocator.destroy_buffer(v->mesh_buffers.indexBuffer);
		m_buffer_allocator.destroy_buffer(v->mesh_buffers.vertexBuffer);
	}

	for (auto& [k, v] : m_images)
	{
		if (v.image == m_error_image.image)
		{
			//dont destroy the default images
			continue;
		}
		m_image_allocator.destroy_image(v);
	}

	for (auto& sampler : m_samplers)
	{
		vkDestroySampler(m_device, sampler, nullptr);
	}
}