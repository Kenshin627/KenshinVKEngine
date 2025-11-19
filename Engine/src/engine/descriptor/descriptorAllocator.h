#pragma once
#include <vulkan/vulkan.h>
#include <deque>
#include <vector>

class DescriptorAllocator
{
public:
	struct PoolSizeRatio
	{
		VkDescriptorType type;
		float ratio;
	};
public:
	DescriptorAllocator() = default;
	~DescriptorAllocator() = default;
	void init(VkDevice device, const std::vector<PoolSizeRatio>& poolsizes, uint32_t setPerPool);
	VkDescriptorPool getFreePool(VkDevice device);
	VkDescriptorPool createPool(VkDevice device);
	VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout* layout);
	void resetPools(VkDevice device);
	void destroy(VkDevice device);
private: 
	std::deque<VkDescriptorPool> mFullPools;
	std::deque<VkDescriptorPool> mFreePools;
	std::vector<PoolSizeRatio> mPoolSizes;
	uint32_t mSetsPerPool;
};