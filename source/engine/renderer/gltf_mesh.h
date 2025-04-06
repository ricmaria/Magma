#pragma once

#include "types.h"
#include <unordered_map>
#include <filesystem>
#include "descriptors.h"
#include "mesh.h"

class GltfMeshLoader;

class GltfMesh : public IRenderable
{
public:
	friend class GltfMeshLoader;
		
	~GltfMesh() { clear_all(); };

	virtual void add_to_render_context(const glm::mat4& topMatrix, RenderContext& ctx);

private:
	GltfMesh() {};

	void clear_all();

	VkDevice m_device;
	BufferAllocator m_buffer_allocator;
	ImageAllocator m_image_allocator;
	AllocatedImage m_error_image;

	// storage for all the data on a given glTF file
	std::unordered_map<std::string, std::shared_ptr<MeshAsset>> m_meshes;
	std::unordered_map<std::string, std::shared_ptr<Node>> m_nodes;
	std::unordered_map<std::string, AllocatedImage> m_images;
	std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> m_material_instances;

	// nodes that dont have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>> m_top_nodes;

	std::vector<VkSampler> m_samplers;

	DescriptorAllocatorGrowable m_descriptor_pool;

	AllocatedBuffer m_material_data_buffer;

};