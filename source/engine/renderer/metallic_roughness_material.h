
#pragma once

#include "types.h"
#include "descriptors.h"

class GLTFMetallic_Roughness
{
public:
	struct MaterialConstants
	{
		glm::vec4 colorFactors;
		glm::vec4 metal_rough_factors;
		//padding, we need it anyway for uniform buffers
		glm::vec4 extra[14];
	};

	struct MaterialResources
	{
		AllocatedImage colorImage;
		VkSampler colorSampler;
		AllocatedImage metalRoughImage;
		VkSampler metalRoughSampler;
		VkBuffer dataBuffer;
		uint32_t dataBufferOffset;
	};

	void build_pipelines(VkDevice device, VkDescriptorSetLayout gpuSceneDataDescriptorLayout, VkFormat drawImageFormat, VkFormat depthImageFormat);
	void clear_resources(VkDevice device);

	MaterialInstance write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator);

private:
	MaterialPipeline _opaquePipeline;
	MaterialPipeline _transparentPipeline;

	VkDescriptorSetLayout _materialLayout;

	DescriptorWriter _writer;
};