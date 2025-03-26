#include "metallic_roughness_material.h"
#include "initializers.h"
#include "pipelines.h"


void GLTFMetallic_Roughness::build_pipelines(VkDevice device, VkDescriptorSetLayout gpuSceneDataDescriptorLayout, VkFormat drawImageFormat, VkFormat depthImageFormat)
{
	VkShaderModule meshFragShader;
	if (!vkutil::load_shader_module("../shaders/mesh.frag.spv", device, &meshFragShader))
	{
		fmt::println("Error when building the triangle fragment shader module");
	}

	VkShaderModule meshVertexShader;
	if (!vkutil::load_shader_module("../shaders/mesh.vert.spv", device, &meshVertexShader))
	{
		fmt::println("Error when building the triangle vertex shader module");
	}

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	DescriptorLayoutBuilder layoutBuilder;
	layoutBuilder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layoutBuilder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	_materialLayout = layoutBuilder.build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layouts[] = { gpuSceneDataDescriptorLayout, _materialLayout };

	VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrixRange;
	mesh_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &newLayout));

	_opaquePipeline.layout = newLayout;
	_transparentPipeline.layout = newLayout;

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;
	pipelineBuilder.set_shaders(meshVertexShader, meshFragShader);
	pipelineBuilder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.set_multisampling_none();
	pipelineBuilder.disable_blending();
	pipelineBuilder.enable_depthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	//render format
	pipelineBuilder.set_color_attachment_format(drawImageFormat);
	pipelineBuilder.set_depth_format(depthImageFormat);

	// use the triangle layout we created
	pipelineBuilder.set_pipeline_layout(newLayout);

	// finally build the pipeline
	_opaquePipeline.pipeline = pipelineBuilder.build_pipeline(device);

	// create the transparent variant
	pipelineBuilder.enable_blending_additive();

	pipelineBuilder.enable_depthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	_transparentPipeline.pipeline = pipelineBuilder.build_pipeline(device);

	vkDestroyShaderModule(device, meshFragShader, nullptr);
	vkDestroyShaderModule(device, meshVertexShader, nullptr);
}

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