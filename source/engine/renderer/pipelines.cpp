#include "pipelines.h"
#include <fstream>
#include "initializers.h"

bool vkutil::load_shader_module(const char* filePath, VkDevice device, VkShaderModule* out_shader_module)
{
	// open the file. With cursor at the end
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	// find what the size of the file is by looking up the location of the cursor
	// because the cursor is at the end, it gives the size directly in bytes
	size_t file_size = (size_t)file.tellg();

	// spirv expects the buffer to be on uint32, so make sure to reserve a int
	// vector big enough for the entire file
	std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

	// put file cursor at beginning
	file.seekg(0);

	// load the entire file into the buffer
	file.read((char*)buffer.data(), file_size);

	// now that the file is loaded into the buffer, we can close it
	file.close();

	// create a new shader module, using the buffer we loaded
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext = nullptr;

	// codeSize has to be in bytes, so multply the ints in the buffer by size of
	// int to know the real size of the buffer
	create_info.codeSize = buffer.size() * sizeof(uint32_t);
	create_info.pCode = buffer.data();

	// check that the creation goes well.
	VkShaderModule shader_module;
	if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
	{
		return false;
	}
	*out_shader_module = shader_module;
	return true;
}

void PipelineBuilder::clear()
{
	// clear all of the structs we need back to 0 with their correct stype

	m_input_assembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

	m_rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

	m_color_blend_attachment = {};

	m_multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };

	m_pipeline_layout = {};

	m_depth_stencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };

	m_render_info = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

	m_shader_stages.clear();
}

VkPipeline PipelineBuilder::build_pipeline(VkDevice device)
{
	// make viewport state from our stored viewport and scissor.
	// at the moment we wont support multiple viewports or scissors
	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = nullptr;

	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	// setup dummy color blending. We arent using transparent objects yet
	// the blending is just "no blend", but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.pNext = nullptr;

	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &m_color_blend_attachment;

	// completely clear VertexInputStateCreateInfo, as we have no need for it
	VkPipelineVertexInputStateCreateInfo vertex_input_info = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	// build the actual pipeline
	// we now use all of the info structs we have been writing into into this one
	// to create the pipeline
	VkGraphicsPipelineCreateInfo pipeline_info = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	
	// connect the renderInfo to the pNext extension mechanism
	pipeline_info.pNext = &m_render_info;

	pipeline_info.stageCount = (uint32_t)m_shader_stages.size();
	pipeline_info.pStages = m_shader_stages.data();
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &m_input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &m_rasterizer;
	pipeline_info.pMultisampleState = &m_multisampling;
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDepthStencilState = &m_depth_stencil;
	pipeline_info.layout = m_pipeline_layout;

	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamic_info = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic_info.pDynamicStates = &state[0];
	dynamic_info.dynamicStateCount = 2;

	pipeline_info.pDynamicState = &dynamic_info;

	// its easy to error out on create graphics pipeline, so we handle it a bit
	// better than the common VK_CHECK case
	VkPipeline new_pipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &new_pipeline) != VK_SUCCESS)
	{
		fmt::println("failed to create pipeline");
		return VK_NULL_HANDLE; // failed to create graphics pipeline
	}
	else
	{
		return new_pipeline;
	}
}

void PipelineBuilder::set_pipeline_layout(VkPipelineLayout pipeline_layout)
{
	m_pipeline_layout = pipeline_layout;
}

void PipelineBuilder::set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
{
	m_shader_stages.clear();

	m_shader_stages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));

	m_shader_stages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void PipelineBuilder::set_input_topology(VkPrimitiveTopology topology)
{
	m_input_assembly.topology = topology;
	// we are not going to use primitive restart on the entire tutorial so leave
	// it on false
	m_input_assembly.primitiveRestartEnable = VK_FALSE;
}

void PipelineBuilder::set_polygon_mode(VkPolygonMode mode)
{
	m_rasterizer.polygonMode = mode;
	m_rasterizer.lineWidth = 1.f;
}

void PipelineBuilder::set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face)
{
	m_rasterizer.cullMode = cull_mode;
	m_rasterizer.frontFace = front_face;
}

void PipelineBuilder::set_multisampling_none()
{
	m_multisampling.sampleShadingEnable = VK_FALSE;
	// multisampling defaulted to no multisampling (1 sample per pixel)
	m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	m_multisampling.minSampleShading = 1.0f;
	m_multisampling.pSampleMask = nullptr;
	// no alpha to coverage either
	m_multisampling.alphaToCoverageEnable = VK_FALSE;
	m_multisampling.alphaToOneEnable = VK_FALSE;
}

void PipelineBuilder::disable_blending()
{
	// default write mask
	m_color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// no blending
	m_color_blend_attachment.blendEnable = VK_FALSE;
}

void PipelineBuilder::set_color_attachment_format(VkFormat format)
{
	m_color_attachment_format = format;
	// connect the format to the renderInfo  structure
	m_render_info.colorAttachmentCount = 1;
	m_render_info.pColorAttachmentFormats = &m_color_attachment_format;
}

void PipelineBuilder::set_depth_format(VkFormat format)
{
	m_render_info.depthAttachmentFormat = format;
}

void PipelineBuilder::enable_depthtest(bool depth_write_enable, VkCompareOp op)
{
	m_depth_stencil.depthTestEnable = VK_TRUE;
	m_depth_stencil.depthWriteEnable = depth_write_enable;
	m_depth_stencil.depthCompareOp = op;
	m_depth_stencil.depthBoundsTestEnable = VK_FALSE;
	m_depth_stencil.stencilTestEnable = VK_FALSE;
	m_depth_stencil.front = {};
	m_depth_stencil.back = {};
	m_depth_stencil.minDepthBounds = 0.f;
	m_depth_stencil.maxDepthBounds = 1.f;
}

void PipelineBuilder::disable_depthtest()
{
	m_depth_stencil.depthTestEnable = VK_FALSE;
	m_depth_stencil.depthWriteEnable = VK_FALSE;
	m_depth_stencil.depthCompareOp = VK_COMPARE_OP_NEVER;
	m_depth_stencil.depthBoundsTestEnable = VK_FALSE;
	m_depth_stencil.stencilTestEnable = VK_FALSE;
	m_depth_stencil.front = {};
	m_depth_stencil.back = {};
	m_depth_stencil.minDepthBounds = 0.f;
	m_depth_stencil.maxDepthBounds = 1.f;
}

void PipelineBuilder::enable_blending_additive()
{
	m_color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_color_blend_attachment.blendEnable = VK_TRUE;
	m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void PipelineBuilder::enable_blending_alphablend()
{
	m_color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	m_color_blend_attachment.blendEnable = VK_TRUE;
	m_color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	m_color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
}