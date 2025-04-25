#include "metallic_roughness_material.h"
#include "initializers.h"
#include "pipelines.h"


void MetallicRoughnessMaterial::build_pipelines(VkDevice device, VkDescriptorSetLayout gpu_scene_data_descriptor_layout, VkFormat draw_image_format, VkFormat depth_image_format)
{
	VkShaderModule mesh_vertex_shader;
	if (!vkutil::load_shader_module("../shaders/mesh.vert.spv", device, &mesh_vertex_shader))
	{
		fmt::println("Error when building the triangle vertex shader module");
	}

	VkShaderModule mesh_frag_shader;
	if (!vkutil::load_shader_module("../shaders/mesh.frag.spv", device, &mesh_frag_shader))
	{
		fmt::println("Error when building the triangle fragment shader module");
	}

	VkPushConstantRange matrix_range{};
	matrix_range.offset = 0;
	matrix_range.size = sizeof(GpuDrawPushConstants);
	matrix_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	DescriptorLayoutBuilder layout_builder;
	layout_builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layout_builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layout_builder.add_binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	m_material_layout = layout_builder.build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	VkDescriptorSetLayout layouts[] = { gpu_scene_data_descriptor_layout, m_material_layout };

	VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
	mesh_layout_info.setLayoutCount = 2;
	mesh_layout_info.pSetLayouts = layouts;
	mesh_layout_info.pPushConstantRanges = &matrix_range;
	mesh_layout_info.pushConstantRangeCount = 1;

	VkPipelineLayout new_layout;
	VK_CHECK(vkCreatePipelineLayout(device, &mesh_layout_info, nullptr, &new_layout));

	m_opaque_pipeline.layout = new_layout;
	m_transparent_pipeline.layout = new_layout;

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	PipelineBuilder pipeline_builder;
	pipeline_builder.set_shaders(mesh_vertex_shader, mesh_frag_shader);
	pipeline_builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipeline_builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	pipeline_builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipeline_builder.set_multisampling_none();
	pipeline_builder.disable_blending();
	pipeline_builder.enable_depthtest(true, VK_COMPARE_OP_LESS);

	//render format
	pipeline_builder.set_color_attachment_format(draw_image_format);
	pipeline_builder.set_depth_format(depth_image_format);

	// use the triangle layout we created
	pipeline_builder.set_pipeline_layout(new_layout);

	// finally build the pipeline
	m_opaque_pipeline.pipeline = pipeline_builder.build_pipeline(device);

	// create the transparent variant
	pipeline_builder.enable_blending_additive();

	pipeline_builder.enable_depthtest(false, VK_COMPARE_OP_LESS_OR_EQUAL);

	m_transparent_pipeline.pipeline = pipeline_builder.build_pipeline(device);

	vkDestroyShaderModule(device, mesh_frag_shader, nullptr);
	vkDestroyShaderModule(device, mesh_vertex_shader, nullptr);
}

MaterialInstance MetallicRoughnessMaterial::write_material(VkDevice device, MaterialPassType pass, const MaterialResources& resources, DescriptorAllocatorGrowable& descriptor_allocator)
{
	MaterialInstance material_instance;
	material_instance.pass_type = pass;
	if (pass == MaterialPassType::Transparent)
	{
		material_instance.pipeline = &m_transparent_pipeline;
	}
	else
	{
		material_instance.pipeline = &m_opaque_pipeline;
	}

	material_instance.descriptor_set = descriptor_allocator.allocate(device, m_material_layout);

	m_writer.clear();
	m_writer.add_buffer_write(0, resources.data_buffer, sizeof(MaterialConstants), resources.data_buffer_offset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	m_writer.add_image_write(1, resources.color_image.image_view, resources.color_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	m_writer.add_image_write(2, resources.metal_rough_image.image_view, resources.metal_rough_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	m_writer.write_set(device, material_instance.descriptor_set);

	return material_instance;
}

void MetallicRoughnessMaterial::clear_resources(VkDevice device)
{
	vkDestroyDescriptorSetLayout(device, m_material_layout, nullptr);
	vkDestroyPipelineLayout(device, m_transparent_pipeline.layout, nullptr);

	vkDestroyPipeline(device, m_transparent_pipeline.pipeline, nullptr);
	vkDestroyPipeline(device, m_opaque_pipeline.pipeline, nullptr);
}