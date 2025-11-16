#include <fstream>
#include "utils.h"
#include "core.h"

void Utils::loadShader(const char* filePath, VkDevice device, VkShaderModule* shaderModule)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		KS_CORE_ASSERT(false, "Failed to open shader file!");
	}
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.flags = 0;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
	createInfo.codeSize = buffer.size();
	createInfo.pNext = nullptr;
	VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, shaderModule));
}