#include "metallic_roughness_material.h"

MaterialInstance GLTFMetallic_Roughness::write_material(VkDevice device, MaterialPass pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptorAllocator)
{
	MaterialInstance matData;
	matData.passType = pass;
	if (pass == MaterialPass::Transparent)
	{
		matData.pipeline = &_transparentPipeline;
	}
	else
	{
		matData.pipeline = &_opaquePipeline;
	}

	matData.materialSet = descriptorAllocator.allocate(device, _materialLayout);


	_writer.clear();
	_writer.write_buffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	_writer.write_image(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	_writer.write_image(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	_writer.update_set(device, matData.materialSet);

	return matData;
}

void GLTFMetallic_Roughness::clear_resources(VkDevice device)
{
	vkDestroyDescriptorSetLayout(device, _materialLayout, nullptr);
	vkDestroyPipelineLayout(device, _transparentPipeline.layout, nullptr);

	vkDestroyPipeline(device, _transparentPipeline.pipeline, nullptr);
	vkDestroyPipeline(device, _opaquePipeline.pipeline, nullptr);
}