#include "core.h"
#include "descriptorSetlayoutBuilder.h"

void DescriptorSetLayoutBuilder::addBinding(uint32_t bindingPoint, VkDescriptorType type, VkShaderStageFlags stage)
{
	VkDescriptorSetLayoutBinding binding{};
	binding.binding = bindingPoint;
	binding.descriptorCount = 1;
	binding.descriptorType = type;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = stage;
	mBindings.push_back(binding);
}

void DescriptorSetLayoutBuilder::clear()
{
	mBindings.clear();
}

VkDescriptorSetLayout DescriptorSetLayoutBuilder::build(VkDevice device, VkShaderStageFlags stage)
{
	VkDescriptorSetLayout layout;
	for (auto& binding : mBindings)
	{
		binding.stageFlags |= stage;
	}
	VkDescriptorSetLayoutCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.bindingCount = mBindings.size();
	info.pBindings = mBindings.data();
	info.pNext = nullptr;	
	VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &layout));
	return layout;
}
