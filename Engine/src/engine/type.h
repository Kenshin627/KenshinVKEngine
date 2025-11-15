#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <deque>
#include <functional>
#include <glm.hpp>

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;
	void push_back(std::function<void()>&& func)
	{
		deletors.push_back(func);
	}
	void flush()
	{
		for (auto iter = deletors.rbegin(); iter != deletors.rend(); iter++)
		{
			(*iter)();
		}
		deletors.clear();
	}
};

struct AllocatedImage
{
	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
	VkFormat format;
	VkExtent3D extent;
};

struct BackGroundPushConstants
{
	glm::vec4 topColor;
	glm::vec4 bottomColor;
};

struct ModelStruct
{
	glm::mat4		modelMatrix;
	VkDeviceAddress vertexAddress;
};