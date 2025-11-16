#pragma once
#include <vulkan/vulkan.h>
#include "type.h"

class VkInitializer
{
public:
	static VkCommandPoolCreateInfo createCommandPoolInfo(int queueFamilyIndex, const VkCommandPoolCreateFlags& f);
	static VkCommandBufferAllocateInfo createCommandBufferInfo(const VkCommandPool& pool);
	static VkSemaphoreCreateInfo createSemaphoreInfo(const VkSemaphoreCreateFlags& f);
	static VkFenceCreateInfo createFenceInfo(const VkFenceCreateFlags& f);
	static VkCommandBufferBeginInfo createCommandBufferBeginInfo(const VkCommandBufferUsageFlags& f);
	static VkSubmitInfo2 createSubmitInfo(VkCommandBufferSubmitInfo* commandBufferInfo, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);
	static VkCommandBufferSubmitInfo createCommandBufferSubmitInfo(const VkCommandBuffer& commandBuffer);
	static VkSemaphoreSubmitInfo createSemaphoreSubmitInfo(const VkSemaphore& semaphore, const VkPipelineStageFlags2& stageFlags);
	static VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);
	static VkImageCreateInfo createImageInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	static VkImageViewCreateInfo createImageViewInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkExtent3D extent);
	static AllocatedBuffer createBuffer(VmaAllocator allocator, size_t size, VkBufferUsageFlags flags, VmaMemoryUsage memoryUsage);
	static AllocatedImage createImage(VkDevice device, VmaAllocator allocator, VkExtent3D extent, VkFormat format, VkImageUsageFlags flags, VkImageAspectFlags aspect);
};