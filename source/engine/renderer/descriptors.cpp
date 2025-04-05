#include "descriptors.h"

void DescriptorLayoutBuilder::add_binding(uint32_t binding, VkDescriptorType type)
{
	VkDescriptorSetLayoutBinding newbind{};
	newbind.binding = binding;
	newbind.descriptorCount = 1;
	newbind.descriptorType = type;

	_bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear()
{
	_bindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shader_stages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
	std::vector<VkDescriptorSetLayoutBinding> local_bindings = _bindings;

	for (auto& b : local_bindings)
	{
		b.stageFlags |= shader_stages;
	}

	VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	info.pNext = pNext;

	info.pBindings = local_bindings.data();
	info.bindingCount = (uint32_t)local_bindings.size();
	info.flags = flags;

	VkDescriptorSetLayout set;
	VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

	return set;
}

void DescriptorAllocator::init_pool(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (const PoolSizeRatio& ratio : pool_ratios)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = ratio.type,
			.descriptorCount = uint32_t(ratio.ratio * max_sets)
			});
	}

	VkDescriptorPoolCreateInfo pool_info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_info.flags = 0;
	pool_info.maxSets = max_sets;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	vkCreateDescriptorPool(device, &pool_info, nullptr, &m_pool);
}

void DescriptorAllocator::clear_descriptors(VkDevice device)
{
	vkResetDescriptorPool(device, m_pool, 0);
}

void DescriptorAllocator::destroy_pool(VkDevice device)
{
	vkDestroyDescriptorPool(device, m_pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
	VkDescriptorSetAllocateInfo alloc_info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	alloc_info.pNext = nullptr;
	alloc_info.descriptorPool = m_pool;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &layout;

	VkDescriptorSet ds;
	VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &ds));

	return ds;
}

VkDescriptorPool DescriptorAllocatorGrowable::get_pool(VkDevice device)
{
	VkDescriptorPool new_pool;
	
	if (m_ready_pools.size() != 0)
	{
		new_pool = m_ready_pools.back();
		m_ready_pools.pop_back();
	}
	else
	{
		//need to create a new pool
		new_pool = create_pool(device, m_sets_per_pool, m_ratios);

		m_sets_per_pool = static_cast<uint32_t>(m_sets_per_pool * 1.5f);
		if (m_sets_per_pool > 4092)
		{
			m_sets_per_pool = 4092;
		}
	}

	return new_pool;
}

VkDescriptorPool DescriptorAllocatorGrowable::create_pool(VkDevice device, uint32_t set_count, std::span<PoolSizeRatio> pool_ratios)
{
	std::vector<VkDescriptorPoolSize> pool_sizes;
	
	for (PoolSizeRatio ratio : pool_ratios)
	{
		pool_sizes.push_back(VkDescriptorPoolSize{
			.type = ratio.type,
			.descriptorCount = uint32_t(ratio.ratio * set_count)
			});
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = set_count;
	pool_info.poolSizeCount = (uint32_t)pool_sizes.size();
	pool_info.pPoolSizes = pool_sizes.data();

	VkDescriptorPool new_pool;
	vkCreateDescriptorPool(device, &pool_info, nullptr, &new_pool);
	return new_pool;
}

void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios)
{
	m_ratios.clear();

	for (auto r : pool_ratios)
	{
		m_ratios.push_back(r);
	}

	VkDescriptorPool new_pool = create_pool(device, max_sets, pool_ratios);

	m_sets_per_pool = static_cast<uint32_t>(max_sets * 1.5f); //grow it next allocation

	m_ready_pools.push_back(new_pool);
}

void DescriptorAllocatorGrowable::clear_pools(VkDevice device)
{
	for (auto p : m_ready_pools)
	{
		vkResetDescriptorPool(device, p, 0);
	}

	for (auto p : m_full_pools)
	{
		vkResetDescriptorPool(device, p, 0);
		m_ready_pools.push_back(p);
	}

	m_full_pools.clear();
}

void DescriptorAllocatorGrowable::destroy_pools(VkDevice device)
{
	for (auto p : m_ready_pools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}

	m_ready_pools.clear();
	
	for (auto p : m_full_pools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	
	m_full_pools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
	//get or create a pool to allocate from
	VkDescriptorPool pool_to_use = get_pool(device);

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.pNext = pNext;
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = pool_to_use;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &layout;

	VkDescriptorSet ds;
	VkResult result = vkAllocateDescriptorSets(device, &alloc_info, &ds);

	//allocation failed. Try again
	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
	{
		m_full_pools.push_back(pool_to_use);

		pool_to_use = get_pool(device);
		alloc_info.descriptorPool = pool_to_use;

		VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &ds));
	}

	m_ready_pools.push_back(pool_to_use);
	
	return ds;
}

void DescriptorWriter::write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
	VkDescriptorBufferInfo& info = m_buffer_infos.emplace_back(VkDescriptorBufferInfo{
		.buffer = buffer,
		.offset = offset,
		.range = size
		});

	VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	write.dstBinding = binding;
	write.dstSet = VK_NULL_HANDLE; //left empty for now until we need to write it
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &info;

	m_writes.push_back(write);
}

void DescriptorWriter::write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
	VkDescriptorImageInfo& info = m_image_infos.emplace_back(VkDescriptorImageInfo{
		.sampler = sampler,
		.imageView = image,
		.imageLayout = layout
		});

	VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

	write.dstBinding = binding;
	write.dstSet = VK_NULL_HANDLE; //left empty for now until we need to write it
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pImageInfo = &info;

	m_writes.push_back(write);
}

void DescriptorWriter::clear()
{
	m_writes.clear();
	m_image_infos.clear();
	m_buffer_infos.clear();
}

void DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
{
	for (VkWriteDescriptorSet& write : m_writes)
	{
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(device, (uint32_t)m_writes.size(), m_writes.data(), 0, nullptr);
}