#pragma once

#include "types.h"

namespace vkutil
{
	bool load_shader_module(const char* file_path, VkDevice device, VkShaderModule* out_shader_module);
}

class PipelineBuilder
{
public:
	PipelineBuilder() { clear(); }

	void set_pipeline_layout(VkPipelineLayout pipeline_layout);
	void set_shaders(VkShaderModule vertex_shader, VkShaderModule fragment_shader);
	void set_input_topology(VkPrimitiveTopology topology);
	void set_polygon_mode(VkPolygonMode mode);
	void set_cull_mode(VkCullModeFlags cull_mode, VkFrontFace front_face);
	void set_multisampling_none();
	void disable_blending();
	void set_color_attachment_format(VkFormat format);
	void set_depth_format(VkFormat format);
	void enable_depthtest(bool depth_write_enable, VkCompareOp op);
	void disable_depthtest();
	void enable_blending_additive();
	void enable_blending_alphablend();

	void clear();

	VkPipeline build_pipeline(VkDevice device);
private:
	std::vector<VkPipelineShaderStageCreateInfo> m_shader_stages;

	VkPipelineInputAssemblyStateCreateInfo m_input_assembly;
	VkPipelineRasterizationStateCreateInfo m_rasterizer;
	VkPipelineColorBlendAttachmentState m_color_blend_attachment;
	VkPipelineMultisampleStateCreateInfo m_multisampling;
	VkPipelineLayout m_pipeline_layout;
	VkPipelineDepthStencilStateCreateInfo m_depth_stencil;
	VkPipelineRenderingCreateInfo m_render_info;
	VkFormat m_color_attachment_format;
};