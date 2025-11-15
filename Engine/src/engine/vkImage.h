#pragma once
#include <vulkan/vulkan.h>	

namespace vkutil 
{
	void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
	void blitImage(VkCommandBuffer cmd, VkImage srcImage, VkImage dstImage, VkExtent3D srcSize, VkExtent3D distSize);
}