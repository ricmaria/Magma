#pragma once

#include "types.h"

class DescriptorLayoutBuilder
{
public:
	void add_binding(uint32_t binding, VkDescriptorType type);
	void clear();
	VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shader_stages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

private:
	std::vector<VkDescriptorSetLayoutBinding> _bindings;
};

class DescriptorAllocator
{
public:
	struct PoolSizeRatio
	{
		VkDescriptorType type;
		float ratio;
	};

	void init_pool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
	void clear_descriptors(VkDevice device);
	void destroy_pool(VkDevice device);

	VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);

private:
	VkDescriptorPool m_pool;
};

class DescriptorAllocatorGrowable
{
public:
	struct PoolSizeRatio
	{
		VkDescriptorType type;
		float ratio;
	};

	void init(VkDevice device, uint32_t initial_sets, std::span<PoolSizeRatio> pool_ratios);
	void clear_pools(VkDevice device);
	void destroy_pools(VkDevice device);

	VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext = nullptr);
private:
	VkDescriptorPool get_pool(VkDevice device);
	VkDescriptorPool create_pool(VkDevice device, uint32_t set_count, std::span<PoolSizeRatio> pool_ratios);

	std::vector<PoolSizeRatio> m_ratios;
	std::vector<VkDescriptorPool> m_full_pools;
	std::vector<VkDescriptorPool> m_ready_pools;
	uint32_t m_sets_per_pool;
};

class DescriptorWriter
{
public:
	void write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
	void write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

	void clear();
	void update_set(VkDevice device, VkDescriptorSet set);

private:
	std::deque<VkDescriptorImageInfo> m_image_infos;
	std::deque<VkDescriptorBufferInfo> m_buffer_infos;
	std::vector<VkWriteDescriptorSet> m_writes;
};