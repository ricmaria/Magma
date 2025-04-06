
#pragma once

#include "types.h"
#include "descriptors.h"

class MetallicRoughnessMaterial
{
public:
	struct MaterialConstants
	{
		glm::vec4 color_factors;
		glm::vec4 metal_rough_factors;
		//padding, we need it anyway for uniform buffers
		glm::vec4 extra[14];
	};

	struct MaterialResources
	{
		AllocatedImage color_image;
		VkSampler color_sampler;
		AllocatedImage metal_rough_image;
		VkSampler metal_rough_sampler;
		VkBuffer data_buffer;
		uint32_t data_buffer_offset;
	};

	void build_pipelines(VkDevice device, VkDescriptorSetLayout gpuScene_data_descriptor_layout, VkFormat draw_image_format, VkFormat depth_image_format);
	void clear_resources(VkDevice device);

	MaterialInstance write_material(VkDevice device, MaterialPassType pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptor_allocator);

private:
	MaterialPipeline m_opaque_pipeline;
	MaterialPipeline m_transparent_pipeline;

	VkDescriptorSetLayout m_material_layout;

	DescriptorWriter m_writer;
};