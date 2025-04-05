#pragma once

#include "gltf_mesh.h"

namespace fastgltf
{
	class Asset;
	struct Image;
}

class GltfMeshLoader
{
public:

	using BuildMaterial = std::function<MaterialInstance(VkDevice, MaterialPassType, const GltfMetallicRoughness::MaterialResources&, DescriptorAllocatorGrowable&)>;
	using UploadMesh = std::function<GpuMeshBuffers(std::span<uint32_t> indices, std::span<Vertex> vertices)>;

	std::string_view file_path;
	VkDevice device;
	BufferAllocator buffer_allocator;
	ImageAllocator image_allocator;
	BuildMaterial build_material;
	UploadMesh upload_mesh;
	AllocatedImage white_image;
	AllocatedImage error_image;
	VkSampler default_sampler;

	std::optional<std::shared_ptr<GltfMesh>> load_gltf_mesh();

	static std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_gltf_meshes(std::filesystem::path file_path, UploadMesh upload_mesh);	// TODO: remove

private:
	static std::optional<AllocatedImage> load_image(ImageAllocator image_allocator, fastgltf::Asset& asset, fastgltf::Image& image);
	
};