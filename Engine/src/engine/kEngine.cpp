#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <chrono>
#include "core.h"
#include "kEngine.h"
#include "vkInitializer.h"
#include "vkImage.h"
#include "utils.h"

constexpr static bool useValidationLayer = true;
KEngine* kEngine = nullptr;

KEngine::KEngine(uint width, uint height)
{
	mWindowExtent.width = width;	
	mWindowExtent.height = height;
}

KEngine::~KEngine()
{
}

void KEngine::init()
{
	KS_CORE_ASSERT(kEngine == nullptr, "Engine already initialized");
	initWindow();
	initVulkan();
	initSwapChain();
	initCommand();
	initSyncStructures();
	initDescriptorSetLayout();
	initDescriptorSet();
	initPipeline();
	kEngine = this;
	mInitialized = true;
}

void KEngine::cleanUp()
{
	if (mInitialized)
	{
		vkDeviceWaitIdle(mDevice);
		for (size_t i = 0; i < FRAME_OVERLAP; i++)
		{
			vkDestroyCommandPool(mDevice, mFrameData[i].commandPool, nullptr);
			vkDestroyFence(mDevice, mFrameData[i].vkFence, nullptr);	
			vkDestroySemaphore(mDevice, mFrameData[i].swapchainSemaphore, nullptr);
			mFrameData[i].deletionQueue.flush();
		}

		for (size_t i = 0; i < mSwapChainImageCount; i++)
		{
			vkDestroySemaphore(mDevice, mSignalSemaphores[i], nullptr);
		}

		mMainDeletionQueue.flush();

		destroySwapChain();
		vkDestroyDevice(mDevice, nullptr);
		vkDestroySurfaceKHR(mVkInstance, mVkSurface, nullptr);
		vkb::destroy_debug_utils_messenger(mVkInstance, mDebugMessage);
		vkDestroyInstance(mVkInstance, nullptr);
		SDL_DestroyWindow(mWindow);
		kEngine = nullptr;
		mInitialized = false;
	}
}

void KEngine::run()
{
	SDL_Event e;
	bool quit = false;
	while (!quit)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_EVENT_QUIT)
			{
				quit = true;
			}
			if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
			{
				mStopRendering = true;
			}
			if (e.type == SDL_EVENT_WINDOW_RESTORED)
			{
				mStopRendering = false;
			}
		}
		if (mStopRendering)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}
		draw();		
	}
}

void KEngine::draw()
{
	//wait lastFrame commandBuffer finish
	VK_CHECK(vkWaitForFences(mDevice, 1, &currentFrame().vkFence, true, UINT64_MAX));
	VK_CHECK(vkResetFences(mDevice, 1, &currentFrame().vkFence));

	//get swapchain image
	uint swapchainImageIndex;
	VkAcquireNextImageInfoKHR acquireInfo{};
	acquireInfo.deviceMask = 1;
	acquireInfo.pNext = nullptr;
	acquireInfo.semaphore = currentFrame().swapchainSemaphore;
	acquireInfo.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	acquireInfo.swapchain = mVkSwapChain;
	acquireInfo.timeout = UINT64_MAX;
	VK_CHECK(vkAcquireNextImage2KHR(mDevice, &acquireInfo, &swapchainImageIndex));

	VK_CHECK(vkResetCommandBuffer(currentFrame().commandBuffer, 0));
	VkCommandBufferBeginInfo beginInfo = VkInitializer::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(currentFrame().commandBuffer, &beginInfo));
	vkutil::transitionImage(currentFrame().commandBuffer, mDrawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	drawBackground();
	vkutil::transitionImage(currentFrame().commandBuffer, mDrawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	drawGeometry();
	vkutil::transitionImage(currentFrame().commandBuffer, mDrawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	vkutil::transitionImage(currentFrame().commandBuffer, mSwapChainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkutil::blitImage(currentFrame().commandBuffer, mDrawImage.image, mSwapChainImages[swapchainImageIndex], mDrawImage.extent, mDrawImage.extent);
	vkutil::transitionImage(currentFrame().commandBuffer, mSwapChainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	
	VK_CHECK(vkEndCommandBuffer(currentFrame().commandBuffer));
	
	VkCommandBufferSubmitInfo commandBufferInfo = VkInitializer::createCommandBufferSubmitInfo(currentFrame().commandBuffer);
	VkSemaphoreSubmitInfo waitSemaphoreInfo = VkInitializer::createSemaphoreSubmitInfo(currentFrame().swapchainSemaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
	VkSemaphoreSubmitInfo signalSemaphoreInfo = VkInitializer::createSemaphoreSubmitInfo(mSignalSemaphores[swapchainImageIndex], VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
	VkSubmitInfo2 submitInfo = VkInitializer::createSubmitInfo(&commandBufferInfo, &signalSemaphoreInfo, &waitSemaphoreInfo);
	VK_CHECK(vkQueueSubmit2(mQueue, 1, &submitInfo, currentFrame().vkFence));

	//present image
	VkPresentInfoKHR presentInfo{};
	presentInfo.pImageIndices = &swapchainImageIndex;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &mVkSwapChain;
	presentInfo.pWaitSemaphores = &mSignalSemaphores[swapchainImageIndex];
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.swapchainCount = 1;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	VK_CHECK(vkQueuePresentKHR(mQueue, &presentInfo));
	currentFrame().deletionQueue.flush();
	mFrameCounter++;
}

void KEngine::initVulkan()
{
	vkb::InstanceBuilder builder;
	auto instanceRes = builder
		.request_validation_layers(useValidationLayer)
		.set_app_name("KEngine")
		.set_engine_name("KEngine")
		.require_api_version(1, 3, 0)
		.use_default_debug_messenger()
		.build();
	vkb::Instance vkbInstace = instanceRes.value();
	mVkInstance = vkbInstace.instance;
	mDebugMessage = vkbInstace.debug_messenger;
	SDL_Vulkan_CreateSurface(mWindow, mVkInstance, nullptr, &mVkSurface);

	//vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	vkb::PhysicalDeviceSelector selector{ vkbInstace };
	auto physicalDeviceRes = selector
		.set_surface(mVkSurface)
		.set_required_features_12(features12)
		.set_required_features_13(features13)
		.select()
		.value();
	mPhysicalDevice = physicalDeviceRes.physical_device;
	vkb::DeviceBuilder deviceBuilder{ physicalDeviceRes };
	auto deviceRes = deviceBuilder.build().value();
	mDevice = deviceRes.device;
	mQueue = deviceRes.get_queue(vkb::QueueType::graphics).value();
	mQueueFamilyIndex = deviceRes.get_queue_index(vkb::QueueType::graphics).value();

	//vma
	VmaAllocatorCreateInfo allocatorInfo = {};	
	allocatorInfo.device = mDevice;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorInfo.physicalDevice = mPhysicalDevice;
	allocatorInfo.instance = mVkInstance;
	vmaCreateAllocator(&allocatorInfo, &mMemAllocator);
	mMainDeletionQueue.push_back([=]() {
		vmaDestroyAllocator(mMemAllocator);
	});
}

void KEngine::initSwapChain()
{
	mSwapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	vkb::SwapchainBuilder builder(mPhysicalDevice, mDevice, mVkSurface);
	auto res = builder
		.set_desired_extent(mWindowExtent.width, mWindowExtent.height)
		.set_desired_format(VkSurfaceFormatKHR{ .format = mSwapChainImageFormat, .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR })
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
	
	vkb::Swapchain vkbSwapChain = res.value();
	mVkSwapChain = vkbSwapChain.swapchain;
	mSwapChainImages = vkbSwapChain.get_images().value();
	mSwapChainImageViews = vkbSwapChain.get_image_views().value();
	mSwapChainExtent = vkbSwapChain.extent;
	mSwapChainImageCount = mSwapChainImages.size();

	mDrawImage.extent.width = mSwapChainExtent.width;
	mDrawImage.extent.height = mSwapChainExtent.height;
	mDrawImage.extent.depth = 1;
	mDrawImage.format = VK_FORMAT_R16G16B16A16_SFLOAT;

	VkImageUsageFlags flags{};
	flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	flags |= VK_IMAGE_USAGE_STORAGE_BIT;
	VkImageCreateInfo imageInfo = VkInitializer::createImageInfo(mDrawImage.format, flags, mDrawImage.extent);
	
	VmaAllocationCreateInfo allocatorInfo {};
	allocatorInfo.flags = 0;
	allocatorInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	allocatorInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaCreateImage(mMemAllocator, &imageInfo, &allocatorInfo, &mDrawImage.image, &mDrawImage.allocation, nullptr);

	VkImageViewCreateInfo imageViewInfo = VkInitializer::createImageViewInfo(mDrawImage.image, mDrawImage.format, VK_IMAGE_ASPECT_COLOR_BIT, mDrawImage.extent);
	vkCreateImageView(mDevice, &imageViewInfo, nullptr, &mDrawImage.imageView);
	mMainDeletionQueue.push_back([=]() {
		vkDestroyImageView(mDevice, mDrawImage.imageView, nullptr);
		vmaDestroyImage(mMemAllocator, mDrawImage.image, mDrawImage.allocation);
	});
}

void KEngine::destroySwapChain()
{
	vkDestroySwapchainKHR(mDevice, mVkSwapChain, nullptr);
	for (auto& imageView : mSwapChainImageViews)
	{
		vkDestroyImageView(mDevice, imageView, nullptr);
	}
}

void KEngine::initCommand()
{
	VkCommandPoolCreateInfo poolInfo = VkInitializer::createCommandPoolInfo(mQueueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	
	for (size_t i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mFrameData[i].commandPool));
		VkCommandBufferAllocateInfo commandInfo = VkInitializer::createCommandBufferInfo(mFrameData[i].commandPool);
		VK_CHECK(vkAllocateCommandBuffers(mDevice, &commandInfo, &mFrameData[i].commandBuffer));
	}
}

void KEngine::initSyncStructures()
{
	VkSemaphoreCreateInfo semaphoreInfo = VkInitializer::createSemaphoreInfo(0);
	VkFenceCreateInfo fenceInfo = VkInitializer::createFenceInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	for (size_t i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(mDevice, &fenceInfo, nullptr, &mFrameData[i].vkFence));
		VK_CHECK(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mFrameData[i].swapchainSemaphore));
	}

	for (size_t i = 0; i < mSwapChainImageCount; i++)
	{
		VkSemaphore signalSemaphore;
		VK_CHECK(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &signalSemaphore));
		mSignalSemaphores.push_back(signalSemaphore);
	}
}

void KEngine::initDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding binding{};
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo info{};
	info.bindingCount = 1;
	info.flags = 0;
	info.pNext = nullptr;
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	info.pBindings = &binding;
	
	VK_CHECK(vkCreateDescriptorSetLayout(mDevice, &info, nullptr, &mComputeDescriptorSetLayout));
	mMainDeletionQueue.push_back([=]() {
		vkDestroyDescriptorSetLayout(mDevice, mComputeDescriptorSetLayout, nullptr);
	});
}

void KEngine::initDescriptorSet()
{
	VkDescriptorPoolSize poolSize;
	poolSize.descriptorCount = 1;
	poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.flags = 0;
	poolInfo.maxSets = 1;
	poolInfo.pNext = nullptr;
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	VK_CHECK(vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mDescriptorPool));

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.descriptorPool = mDescriptorPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pNext = nullptr;
	allocateInfo.pSetLayouts = &mComputeDescriptorSetLayout;
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	vkAllocateDescriptorSets(mDevice, &allocateInfo, &mComputeDescriptorSet);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageInfo.imageView = mDrawImage.imageView;
	imageInfo.sampler = VK_NULL_HANDLE;

	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.dstSet = mComputeDescriptorSet;
	writeDescriptorSet.pBufferInfo = nullptr;
	writeDescriptorSet.pImageInfo = &imageInfo;
	writeDescriptorSet.pNext = nullptr;
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	vkUpdateDescriptorSets(mDevice, 1, &writeDescriptorSet, 0, nullptr);
	mMainDeletionQueue.push_back([=]() {
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
	});
}

void KEngine::initPipeline()
{
	initComputePipeline();
	initGraphicPipeline();
}

void KEngine::initComputePipeline()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.pNext = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	VkPushConstantRange rangeInfo{};
	rangeInfo.offset = 0;
	rangeInfo.size = sizeof(BackGroundPushConstants);
	rangeInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	
	pipelineLayoutInfo.pPushConstantRanges = &rangeInfo;
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &mComputeDescriptorSetLayout;

	VK_CHECK(vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mComputePipelineLayout));
	VkShaderModule computeShaderModule;
	Utils::loadShader("src/shaders/spirv/grid.comp.spirv", mDevice, &computeShaderModule);
	VkPipelineShaderStageCreateInfo stageInfo{};
	stageInfo.flags = 0;
	stageInfo.module = computeShaderModule;
	stageInfo.pName = "main";
	stageInfo.pNext = nullptr;
	stageInfo.pSpecializationInfo = nullptr;
	stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	VkComputePipelineCreateInfo computePipelineInfo{};
	computePipelineInfo.flags = 0;
	computePipelineInfo.layout = mComputePipelineLayout;
	computePipelineInfo.pNext = nullptr;
	computePipelineInfo.stage = stageInfo;
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

	VK_CHECK(vkCreateComputePipelines(mDevice, nullptr, 1, &computePipelineInfo, nullptr, &mComputePipeline));

	vkDestroyShaderModule(mDevice, computeShaderModule, nullptr);
	mMainDeletionQueue.push_back([=]() {
		vkDestroyPipelineLayout(mDevice, mComputePipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mComputePipeline, nullptr);
	});
}

void KEngine::initGraphicPipeline()
{
	VkPipelineShaderStageCreateInfo vertexStages[2] = { {}, {} };
	VkShaderModule vertexShaderModule;
	Utils::loadShader("src/shaders/spirv/triangle.vert.spirv", mDevice, &vertexShaderModule);
	vertexStages[0].flags = 0;
	vertexStages[0].module = vertexShaderModule;
	vertexStages[0].pName = "main";
	vertexStages[0].pNext = nullptr;
	vertexStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	VkShaderModule fragmentShaderModule;
	Utils::loadShader("src/shaders/spirv/triangle.frag.spirv", mDevice, &fragmentShaderModule);
	vertexStages[1].flags = 0;
	vertexStages[1].module = fragmentShaderModule;
	vertexStages[1].pName = "main";
	vertexStages[1].pNext = nullptr;
	vertexStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	vertexStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	VkPushConstantRange pcRange{};
	pcRange.offset = 0;
	pcRange.size = sizeof(ModelStruct);
	pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.flags = 0;
	layoutInfo.pNext = nullptr;
	layoutInfo.pPushConstantRanges = &pcRange;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pSetLayouts = nullptr;
	layoutInfo.setLayoutCount = 0;
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	vkCreatePipelineLayout(mDevice, &layoutInfo, nullptr, &mGraphicPipelineLayout);

	VkPipelineColorBlendAttachmentState blendAttachmentState{};
	blendAttachmentState.blendEnable = VK_FALSE;
	blendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo blend{};
	blend.attachmentCount = 1;
	blend.flags = 0;
	blend.pAttachments = &blendAttachmentState;
	blend.logicOpEnable = VK_FALSE;
	blend.pNext = nullptr;
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.depthTestEnable = VK_FALSE;
	depthStencilState.depthWriteEnable = VK_FALSE;
	depthStencilState.maxDepthBounds = 1.0f;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

	VkPipelineDynamicStateCreateInfo dynamicState{};
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	dynamicState.dynamicStateCount = 2;
	dynamicState.flags = 0;
	dynamicState.pDynamicStates = dynamicStates;
	dynamicState.pNext = nullptr;
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo assembly{};
	assembly.flags = 0;
	assembly.pNext = nullptr;
	assembly.primitiveRestartEnable = VK_FALSE;
	assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineMultisampleStateCreateInfo msInfo{};
	msInfo.alphaToCoverageEnable = VK_FALSE;
	msInfo.alphaToOneEnable = VK_FALSE;
	msInfo.flags = 0;
	msInfo.pNext = nullptr;
	msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	msInfo.sampleShadingEnable = VK_FALSE;
	msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	VkPipelineRasterizationStateCreateInfo rasterInfo{};
	rasterInfo.cullMode  = VK_CULL_MODE_NONE;
	rasterInfo.depthBiasEnable = VK_FALSE;
	rasterInfo.depthClampEnable = VK_FALSE;
	rasterInfo.flags = 0;
	rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterInfo.lineWidth = 1.0f;
	rasterInfo.pNext = nullptr;
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;;
	rasterInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	VkPipelineViewportStateCreateInfo viewPortInfo{};
	viewPortInfo.flags = 0;
	viewPortInfo.pNext = nullptr;
	viewPortInfo.pScissors = nullptr;
	viewPortInfo.pViewports = nullptr;
	viewPortInfo.scissorCount = 1;
	viewPortInfo.viewportCount = 1;
	viewPortInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

	VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
	VkPipelineRenderingCreateInfo renderingInfo{};
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pNext = nullptr;
	renderingInfo.pColorAttachmentFormats = &mDrawImage.format;
	renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingInfo.viewMask = 0;

	VkPipelineVertexInputStateCreateInfo inputInfo{};
	inputInfo.flags = 0;
	inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.flags = 0;
	pipelineInfo.layout = mGraphicPipelineLayout;
	pipelineInfo.pColorBlendState = &blend;
	pipelineInfo.pDepthStencilState = &depthStencilState;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pInputAssemblyState = &assembly;
	pipelineInfo.pMultisampleState = &msInfo;
	pipelineInfo.pNext = &renderingInfo;
	pipelineInfo.pRasterizationState = &rasterInfo;
	pipelineInfo.pStages = vertexStages;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pVertexInputState = &inputInfo;
	pipelineInfo.pViewportState = &viewPortInfo;
	pipelineInfo.renderPass = nullptr;
	pipelineInfo.stageCount = 2;
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	vkCreateGraphicsPipelines(mDevice, nullptr, 1, &pipelineInfo, nullptr, &mGraphicPipeline);

	vkDestroyShaderModule(mDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(mDevice, fragmentShaderModule, nullptr);
	mMainDeletionQueue.push_back([=]() {
		vkDestroyPipelineLayout(mDevice, mGraphicPipelineLayout, nullptr);
		vkDestroyPipeline(mDevice, mGraphicPipeline, nullptr);
		});
}

FrameData& KEngine::currentFrame()
{
	return mFrameData[(mFrameCounter + 1) % FRAME_OVERLAP];
}

void KEngine::drawBackground()
{
	vkCmdBindPipeline(currentFrame().commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline);
	vkCmdBindDescriptorSets(currentFrame().commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipelineLayout, 0, 1, &mComputeDescriptorSet, 0, nullptr);

	BackGroundPushConstants pushConstants;
	pushConstants.topColor = { 1.0, 1.0, 1.0, 1.0 };
	pushConstants.bottomColor = { 1.0, 1.0, 0.0, 1.0 };
	VkPushConstantsInfo pcInfo{};
	pcInfo.layout = mComputePipelineLayout;
	pcInfo.offset = 0;
	pcInfo.pNext = nullptr;
	pcInfo.pValues = &pushConstants;
	pcInfo.size = sizeof(BackGroundPushConstants);
	pcInfo.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pcInfo.sType = VK_STRUCTURE_TYPE_PUSH_CONSTANTS_INFO;
	vkCmdPushConstants(currentFrame().commandBuffer, mComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(BackGroundPushConstants), &pushConstants);

	vkCmdDispatch(currentFrame().commandBuffer, (uint32_t)std::ceil(mDrawImage.extent.width / 16.f), (uint32_t)std::ceil(mDrawImage.extent.height / 16.f), 1);
}

void KEngine::drawGeometry()
{
	vkCmdBindPipeline(currentFrame().commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicPipeline);

	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = mDrawImage.extent.width;
	viewport.height = mDrawImage.extent.height;
	vkCmdSetViewport(currentFrame().commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};	
	scissor.offset = { 0, 0 };
	scissor.extent = { mDrawImage.extent.width, mDrawImage.extent.height };
	vkCmdSetScissor(currentFrame().commandBuffer, 0, 1, &scissor);
	VkRenderingAttachmentInfo attachmentInfo{};
	attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentInfo.imageView = mDrawImage.imageView;
	attachmentInfo.pNext = nullptr;
	attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	VkRenderingInfo renderingInfo{};
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.flags = 0;
	renderingInfo.layerCount = 1;
	renderingInfo.pColorAttachments = &attachmentInfo;
	renderingInfo.pDepthAttachment = nullptr;
	renderingInfo.pNext = nullptr;
	renderingInfo.pStencilAttachment = nullptr;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = { mDrawImage.extent.width, mDrawImage.extent.height };
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.viewMask = 0;
	vkCmdBeginRendering(currentFrame().commandBuffer, &renderingInfo);
	vkCmdDraw(currentFrame().commandBuffer, 3, 1, 0, 0);
	vkCmdEndRendering(currentFrame().commandBuffer);
}

void KEngine::initWindow()
{
	KS_CORE_ASSERT(!mInitialized, "Engine already initialized");
	bool initSDL = SDL_Init(SDL_INIT_VIDEO);
	KS_CORE_ASSERT(initSDL, "Failed to initialize SDL3");
	mWindow = SDL_CreateWindow("KEngine", mWindowExtent.width, mWindowExtent.height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
}
