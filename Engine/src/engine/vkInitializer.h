#pragma once
#include <vulkan/vulkan.h>

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
};