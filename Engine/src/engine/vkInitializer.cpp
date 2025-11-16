#include "vkInitializer.h"
#include "core.h"

VkCommandPoolCreateInfo VkInitializer::createCommandPoolInfo(int queueFamilyIndex, const VkCommandPoolCreateFlags& f)
{
	VkCommandPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = f;
	info.queueFamilyIndex = queueFamilyIndex;
	info.pNext = nullptr;
	return info;
}

VkCommandBufferAllocateInfo VkInitializer::createCommandBufferInfo(const VkCommandPool& pool)
{
	VkCommandBufferAllocateInfo info{};
	info.commandBufferCount = 1;
	info.commandPool = pool;
	info.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.pNext = nullptr;
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	return info;
}

VkSemaphoreCreateInfo VkInitializer::createSemaphoreInfo(const VkSemaphoreCreateFlags& f)
{
	VkSemaphoreCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	info.flags = f;
	info.pNext = nullptr;
	return info;
}

VkFenceCreateInfo VkInitializer::createFenceInfo(const VkFenceCreateFlags& f)
{
	VkFenceCreateInfo info{};
	info.flags = f;
	info.pNext = nullptr;
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	return info;
}

VkCommandBufferBeginInfo VkInitializer::createCommandBufferBeginInfo(const VkCommandBufferUsageFlags& f)
{
	VkCommandBufferBeginInfo info{};
	info.flags = f;
	info.pNext = nullptr;
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	return info;
}

VkSubmitInfo2 VkInitializer::createSubmitInfo(VkCommandBufferSubmitInfo* commandBufferInfo, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
	VkSubmitInfo2 info{};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	info.pNext = nullptr;
	info.commandBufferInfoCount = 1;
	info.pCommandBufferInfos = commandBufferInfo;;
	info.signalSemaphoreInfoCount = signalSemaphoreInfo ? 1 : 0;
	info.pSignalSemaphoreInfos = signalSemaphoreInfo;
	info.waitSemaphoreInfoCount = waitSemaphoreInfo ? 1 : 0;
	info.pWaitSemaphoreInfos = waitSemaphoreInfo;
	return info;
}

VkCommandBufferSubmitInfo VkInitializer::createCommandBufferSubmitInfo(const VkCommandBuffer& commandBuffer)
{
	VkCommandBufferSubmitInfo info;
	info.commandBuffer = commandBuffer;
	info.deviceMask = 0;
	info.pNext = nullptr;
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	return info;
}

VkSemaphoreSubmitInfo VkInitializer::createSemaphoreSubmitInfo(const VkSemaphore& semaphore, const VkPipelineStageFlags2& stageFlags)
{
	VkSemaphoreSubmitInfo info{};
	info.deviceIndex = 0;
	info.pNext = nullptr;
	info.semaphore = semaphore;
	info.stageMask = stageFlags;
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	info.value = 0;
	return info;
}

VkImageSubresourceRange VkInitializer::imageSubresourceRange(VkImageAspectFlags aspectMask)
{
	VkImageSubresourceRange subImage{};
	subImage.aspectMask = aspectMask;
	subImage.baseMipLevel = 0;
	subImage.levelCount = VK_REMAINING_MIP_LEVELS;
	subImage.baseArrayLayer = 0;
	subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;
	return subImage;
}

VkImageCreateInfo VkInitializer::createImageInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
{
	VkImageCreateInfo info{};
	info.arrayLayers = 1;
	info.extent = extent;
	info.format = format;
	info.imageType = VK_IMAGE_TYPE_2D;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.mipLevels = 1;
	info.pNext = nullptr;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.usage = usageFlags;
	return info;
}

VkImageViewCreateInfo VkInitializer::createImageViewInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkExtent3D extent)
{
	VkImageViewCreateInfo info{};
	info.format = format;
	info.image = image;
	info.pNext = nullptr;
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.subresourceRange = imageSubresourceRange(aspectFlags);
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	return info;
}

AllocatedBuffer VkInitializer::createBuffer(VmaAllocator allocator, size_t size, VkBufferUsageFlags flags, VmaMemoryUsage memoryUsage)
{
	AllocatedBuffer newBuffer;
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.flags = 0;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = size;
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.usage = flags;
	
	VmaAllocationCreateInfo vmaInfo{};
	vmaInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	//vmaInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	vmaInfo.usage = memoryUsage;
	VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &vmaInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.allocationInfo));
	return newBuffer;
}

AllocatedImage VkInitializer::createImage(VkDevice device, VmaAllocator allocator, VkExtent3D extent, VkFormat format, VkImageUsageFlags flags, VkImageAspectFlags aspect)
{
	AllocatedImage newImage;	
	newImage.extent.width  = extent.width;
	newImage.extent.height = extent.height;
	newImage.extent.depth  = extent.depth;
	newImage.format = format;
	VkImageCreateInfo imageInfo = VkInitializer::createImageInfo(newImage.format, flags, newImage.extent);
	VmaAllocationCreateInfo allocatorInfo{};
	allocatorInfo.flags = 0;
	allocatorInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	allocatorInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaCreateImage(allocator, &imageInfo, &allocatorInfo, &newImage.image, &newImage.allocation, &newImage.allocInfo);
	VkImageViewCreateInfo imageViewInfo = VkInitializer::createImageViewInfo(newImage.image, newImage.format, aspect, newImage.extent);
	vkCreateImageView(device, &imageViewInfo, nullptr, &newImage.imageView);
	return newImage;
}
