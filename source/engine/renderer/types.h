// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>


#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            fmt::println("Detected Vulkan error: {}", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)

struct AllocatedBuffer
{
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo info;
};

struct Vertex
{
	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
};

// holds the resources needed for a mesh
struct GpuMeshBuffers
{
	AllocatedBuffer index_buffer;
	AllocatedBuffer vertex_buffer;
	VkDeviceAddress vertex_buffer_address;
};

// push constants for our mesh object draws
struct GpuDrawPushConstants
{
	glm::mat4 world_matrix;
	VkDeviceAddress vertex_buffer;
};

enum class MaterialPassType :uint8_t
{
	MainColor,
	Transparent,
	Other
};

struct MaterialPipeline
{
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct MaterialInstance
{
	MaterialPipeline* pipeline;
	VkDescriptorSet material_set;
	MaterialPassType pass_type;
};

struct AllocatedImage
{
	VkImage image;
	VkImageView image_view;
	VmaAllocation allocation;
	VkExtent3D image_extent;
	VkFormat image_format;
};

struct Bounds
{
	glm::vec3 origin;
	float sphere_radius;
	glm::vec3 extents;
};

struct GpuRenderObject
{
	uint32_t index_count;
	uint32_t first_index;
	VkBuffer index_buffer;

	MaterialInstance* material;
	Bounds bounds;
	glm::mat4 transform;
	VkDeviceAddress vertex_buffer_address;
};

struct RenderContext
{
	std::vector<GpuRenderObject> opaque_surfaces;
	std::vector<GpuRenderObject> transparent_surfaces;
};

// base class for a renderable dynamic object
struct IRenderable
{
	virtual void add_to_render_context(const glm::mat4& top_matrix, RenderContext& context) = 0;
};

// implementation of a drawable scene node.
// the scene node can hold children and will also keep a transform to propagate
// to them
struct Node : public IRenderable
{
	// parent pointer must be a weak pointer to avoid circular dependencies
	std::weak_ptr<Node> parent;
	std::vector<std::shared_ptr<Node>> children;

	glm::mat4 local_transform;
	glm::mat4 world_transform;

	void refresh_transform(const glm::mat4& parentMatrix)
	{
		world_transform = parentMatrix * local_transform;
		for (auto& child : children)
		{
			child->refresh_transform(world_transform);
		}
	}

	virtual void add_to_render_context(const glm::mat4& topMatrix, RenderContext& ctx)
	{
		// draw children
		for (auto& child : children)
		{
			child->add_to_render_context(topMatrix, ctx);
		}
	}
};

struct BufferAllocator
{
	using CreateFunc = std::function<AllocatedBuffer(size_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage)>;
	using DestroyFunc = std::function<void(const AllocatedBuffer& buffer)>;

	CreateFunc create_buffer;
	DestroyFunc destroy_buffer;

	operator bool() const { return create_buffer && destroy_buffer; }
};

struct ImageAllocator
{
	using CreateFunc = std::function<AllocatedImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)>;
	using DestroyFunc = std::function<void(const AllocatedImage& image)>;

	CreateFunc create_image;
	DestroyFunc destroy_image;

	operator bool() const { return create_image && destroy_image; }
};