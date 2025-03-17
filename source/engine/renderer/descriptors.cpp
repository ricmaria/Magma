#include <vk_descriptors.h>

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

VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
	std::vector<VkDescriptorSetLayoutBinding> local_bindings = _bindings;

	for (auto& b : local_bindings)
	{
		b.stageFlags |= shaderStages;
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

void DescriptorAllocator::init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	for (const PoolSizeRatio& ratio : poolRatios)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = ratio.type,
			.descriptorCount = uint32_t(ratio.ratio * maxSets)
			});
	}

	VkDescriptorPoolCreateInfo pool_info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_info.flags = 0;
	pool_info.maxSets = maxSets;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	vkCreateDescriptorPool(device, &pool_info, nullptr, &_pool);
}

void DescriptorAllocator::clear_descriptors(VkDevice device)
{
	vkResetDescriptorPool(device, _pool, 0);
}

void DescriptorAllocator::destroy_pool(VkDevice device)
{
	vkDestroyDescriptorPool(device, _pool, nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
	VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocInfo.pNext = nullptr;
	allocInfo.descriptorPool = _pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet ds;
	VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

	return ds;
}

VkDescriptorPool DescriptorAllocatorGrowable::get_pool(VkDevice device)
{
	VkDescriptorPool newPool;
	
	if (_readyPools.size() != 0)
	{
		newPool = _readyPools.back();
		_readyPools.pop_back();
	}
	else
	{
		//need to create a new pool
		newPool = create_pool(device, _setsPerPool, _ratios);

		_setsPerPool = static_cast<uint32_t>(_setsPerPool * 1.5f);
		if (_setsPerPool > 4092)
		{
			_setsPerPool = 4092;
		}
	}

	return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::create_pool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	
	for (PoolSizeRatio ratio : poolRatios)
	{
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = ratio.type,
			.descriptorCount = uint32_t(ratio.ratio * setCount)
			});
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = setCount;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
	return newPool;
}

void DescriptorAllocatorGrowable::init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
	_ratios.clear();

	for (auto r : poolRatios)
	{
		_ratios.push_back(r);
	}

	VkDescriptorPool newPool = create_pool(device, maxSets, poolRatios);

	_setsPerPool = static_cast<uint32_t>(maxSets * 1.5f); //grow it next allocation

	_readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::clear_pools(VkDevice device)
{
	for (auto p : _readyPools)
	{
		vkResetDescriptorPool(device, p, 0);
	}

	for (auto p : _fullPools)
	{
		vkResetDescriptorPool(device, p, 0);
		_readyPools.push_back(p);
	}

	_fullPools.clear();
}

void DescriptorAllocatorGrowable::destroy_pools(VkDevice device)
{
	for (auto p : _readyPools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}

	_readyPools.clear();
	
	for (auto p : _fullPools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	
	_fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
	//get or create a pool to allocate from
	VkDescriptorPool poolToUse = get_pool(device);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = pNext;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = poolToUse;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet ds;
	VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

	//allocation failed. Try again
	if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
	{
		_fullPools.push_back(poolToUse);

		poolToUse = get_pool(device);
		allocInfo.descriptorPool = poolToUse;

		VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
	}

	_readyPools.push_back(poolToUse);
	
	return ds;
}

void DescriptorWriter::write_buffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
	VkDescriptorBufferInfo& info = _bufferInfos.emplace_back(VkDescriptorBufferInfo{
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

	_writes.push_back(write);
}

void DescriptorWriter::write_image(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
	VkDescriptorImageInfo& info = _imageInfos.emplace_back(VkDescriptorImageInfo{
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

	_writes.push_back(write);
}

void DescriptorWriter::clear()
{
	_writes.clear();
	_imageInfos.clear();
	_bufferInfos.clear();
}

void DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
{
	for (VkWriteDescriptorSet& write : _writes)
	{
		write.dstSet = set;
	}

	vkUpdateDescriptorSets(device, (uint32_t)_writes.size(), _writes.data(), 0, nullptr);
}