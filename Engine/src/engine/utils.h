#pragma once
#include <vulkan/vulkan.h>

namespace Utils
{
	void loadShader(const char* filePath, VkDevice device, VkShaderModule* shaderModule);
}