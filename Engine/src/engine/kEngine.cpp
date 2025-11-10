#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <chrono>
#include "core.h"
#include "kEngine.h"
#include "vkInitializer.h"
#include "vkImage.h"

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
			
		}

		for (size_t i = 0; i < mSwapChainImageCount; i++)
		{
			vkDestroySemaphore(mDevice, mSignalSemaphores[i], nullptr);
		}
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

	//acquireInfo.fence = VK_NULL_HANDLE;
	acquireInfo.pNext = nullptr;
	acquireInfo.semaphore = currentFrame().swapchainSemaphore;
	acquireInfo.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	acquireInfo.swapchain = mVkSwapChain;
	acquireInfo.timeout = UINT64_MAX;
	VK_CHECK(vkAcquireNextImage2KHR(mDevice, &acquireInfo, &swapchainImageIndex));

	VK_CHECK(vkResetCommandBuffer(currentFrame().commandBuffer, 0));
	VkCommandBufferBeginInfo beginInfo = VkInitializer::createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(currentFrame().commandBuffer, &beginInfo));
	vkutil::transitionImage(currentFrame().commandBuffer, mSwapChainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	VkClearColorValue clearValue;
	float flash = std::abs(std::sin(mFrameCounter / 120.f));
	clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

	VkImageSubresourceRange clearRange = VkInitializer::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

	//clear image
	vkCmdClearColorImage(currentFrame().commandBuffer, mSwapChainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	//make the swapchain image into presentable mode
	vkutil::transitionImage(currentFrame().commandBuffer, mSwapChainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	
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

FrameData& KEngine::currentFrame()
{
	return mFrameData[(mFrameCounter + 1) % FRAME_OVERLAP];
}

void KEngine::initWindow()
{
	KS_CORE_ASSERT(!mInitialized, "Engine already initialized");
	bool initSDL = SDL_Init(SDL_INIT_VIDEO);
	KS_CORE_ASSERT(initSDL, "Failed to initialize SDL3");
	mWindow = SDL_CreateWindow("KEngine", mWindowExtent.width, mWindowExtent.height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
}
