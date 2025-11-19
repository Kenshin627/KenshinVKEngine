#include "descriptorAllocator.h"
#include "core.h"

void DescriptorAllocator::init(VkDevice device, const std::vector<PoolSizeRatio>& poolsizes, uint32_t setPerPool)
{
	mSetsPerPool = setPerPool;
	mPoolSizes = poolsizes;
	VkDescriptorPool newPool = createPool(device);
	mFreePools.push_back(newPool);
	mSetsPerPool *= 1.5;
}

VkDescriptorPool DescriptorAllocator::getFreePool(VkDevice device)
{
	VkDescriptorPool freePool;
	if (mFreePools.size() > 0)
	{
		freePool = mFreePools.back();
		mFreePools.pop_back();
	}
	else
	{
		freePool = createPool(device);
		mSetsPerPool *= 1.5;
	}
	return freePool;
}

VkDescriptorPool DescriptorAllocator::createPool(VkDevice device)
{
	VkDescriptorPool newPool = VK_NULL_HANDLE;
	std::vector<VkDescriptorPoolSize> vkPoolSizes;
	vkPoolSizes.reserve(mPoolSizes.size());
	for (auto& item : mPoolSizes)
	{
		vkPoolSizes.emplace_back(VkDescriptorPoolSize{
			.type = item.type,
			.descriptorCount = static_cast<uint32_t>(item.ratio * mSetsPerPool)
			});
	}
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.maxSets = mSetsPerPool;
	poolInfo.poolSizeCount = vkPoolSizes.size();
	poolInfo.pPoolSizes = vkPoolSizes.data();
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &newPool));
	return newPool;
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout* layout)
{
	VkDescriptorSet descriptorSet;
	VkDescriptorPool freePool = getFreePool(device);
	VkDescriptorSetAllocateInfo info{};
	info.descriptorPool = freePool;
	info.descriptorSetCount = 1;
	info.pSetLayouts = layout;
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	VkResult allocateRes = vkAllocateDescriptorSets(device, &info, &descriptorSet);
	if (allocateRes == VK_ERROR_OUT_OF_POOL_MEMORY || allocateRes == VK_ERROR_FRAGMENTED_POOL)
	{
		mFullPools.push_back(freePool);
		freePool = getFreePool(device);
		info.descriptorPool = freePool;
		VK_CHECK(vkAllocateDescriptorSets(device, &info, &descriptorSet));
	}
	mFreePools.push_back(freePool);
	return descriptorSet;
}

void DescriptorAllocator::resetPools(VkDevice device)
{
	for (auto& p : mFreePools)
	{
		vkResetDescriptorPool(device, p, 0);
	}
	for (auto& p : mFullPools)
	{
		vkResetDescriptorPool(device, p, 0);
		mFreePools.push_back(p);
	}
	mFullPools.clear();
}

void DescriptorAllocator::destroy(VkDevice device)
{
	for (auto& p : mFreePools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	mFreePools.clear();
	for (auto& p : mFullPools)
	{
		vkDestroyDescriptorPool(device, p, nullptr);
	}
	mFullPools.clear();
}
