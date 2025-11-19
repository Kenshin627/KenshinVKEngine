#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class DescriptorSetLayoutBuilder
{
public:
	DescriptorSetLayoutBuilder() = default;
	~DescriptorSetLayoutBuilder() = default;
	void addBinding(uint32_t bindingPoint, VkDescriptorType type, VkShaderStageFlags stage);
	void clear();
	VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags stage);
private:
	std::vector<VkDescriptorSetLayoutBinding> mBindings;
};