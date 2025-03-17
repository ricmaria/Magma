#pragma once

#include "types.h"
#include <unordered_map>
#include <filesystem>
#include "descriptors.h"
#include "metallic_roughness_material.h"

struct GLTFMaterial
{
	MaterialInstance data;
};

struct GeoSurface
{
	uint32_t startIndex;
	uint32_t count;
	Bounds bounds;
	std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset
{
	std::string name;

	std::vector<GeoSurface> surfaces;
	GPUMeshBuffers meshBuffers;
};

struct MeshNode : public Node
{
	std::shared_ptr<MeshAsset> mesh;

	virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};

//forward declaration
class VulkanEngine;

namespace fastgltf
{
	class Asset;
	struct Image;
}

struct LoadedGLTF : public IRenderable
{
	// storage for all the data on a given glTF file
	std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
	std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
	std::unordered_map<std::string, AllocatedImage> images;
	std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

	// nodes that dont have a parent, for iterating through the file in tree order
	std::vector<std::shared_ptr<Node>> topNodes;

	std::vector<VkSampler> samplers;

	DescriptorAllocatorGrowable descriptorPool;

	AllocatedBuffer materialDataBuffer;

	using BuildMaterial = std::function<MaterialInstance(VkDevice, MaterialPass, const GLTFMetallic_Roughness::MaterialResources&, DescriptorAllocatorGrowable&)>;
	using UploadMesh = std::function<GPUMeshBuffers(std::span<uint32_t> indices, std::span<Vertex> vertices)>;

	struct LoadedGLTFParams
	{
		VkDevice device;
		BufferAllocator bufferAllocator;
		ImageAllocator imageAllocator;
		AllocatedImage errorImage;
	};

	LoadedGLTF(LoadedGLTFParams params) :
		_device(params.device), _bufferAllocator(params.bufferAllocator), _imageAllocator(params.imageAllocator), _errorImage(params.errorImage) { materialDataBuffer = {}; };
	~LoadedGLTF() { clear_all(); };

	virtual void draw(const glm::mat4& topMatrix, DrawContext& ctx);

	static std::optional<std::vector<std::shared_ptr<MeshAsset>>> load_gltf_meshes(std::filesystem::path filePath, UploadMesh uploadMesh);	// TODO: remove
	
	struct LoadGLTFParams
	{
		std::string_view filePath;
		VkDevice device;
		BufferAllocator bufferAllocator;
		ImageAllocator imageAllocator;
		BuildMaterial buildMaterial;
		UploadMesh uploadMesh;
		AllocatedImage whiteImage;
		AllocatedImage errorImage;
		VkSampler defaultSampler;
	};

	static std::optional<std::shared_ptr<LoadedGLTF>> load_gltf(const LoadGLTFParams& params);

	static std::optional<AllocatedImage> load_image(ImageAllocator imageAllocator, fastgltf::Asset& asset, fastgltf::Image& image);

private:

	void clear_all();

	VkDevice _device;
	BufferAllocator _bufferAllocator;
	ImageAllocator _imageAllocator;
	AllocatedImage _errorImage;
};