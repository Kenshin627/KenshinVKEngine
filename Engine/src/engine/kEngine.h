#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "typedef.h"

constexpr static int FRAME_OVERLAP = 2;

struct FrameData
{
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkFence vkFence;
	VkSemaphore swapchainSemaphore;
	//VkSemaphore renderSemaphore;
};

struct SDL_Window;
class KEngine
{
public:
	KEngine(uint width, uint height);
	~KEngine();
	void init();
	void cleanUp();
	void run();
	void draw();
private:
	void initWindow();
	void initVulkan();
	void initSwapChain();
	void destroySwapChain();
	void initCommand();
	void initSyncStructures();
	FrameData& currentFrame();
private:
	bool					  mInitialized		{ false     };
	uint					  mFrameCounter		{ 0		    };
	SDL_Window*				  mWindow			{ nullptr   };
	VkInstance				  mVkInstance		{ nullptr   };
	VkDebugUtilsMessengerEXT  mDebugMessage		{ nullptr   };
	VkPhysicalDevice		  mPhysicalDevice	{ nullptr   };
	VkDevice				  mDevice		    { nullptr   };
	VkExtent2D				  mWindowExtent		{ 1280, 720 };
	bool					  mStopRendering	{ false		};
	VkSurfaceKHR			  mVkSurface		{ nullptr	};
	VkSwapchainKHR			  mVkSwapChain		{ nullptr	};
	std::vector<VkImage>	  mSwapChainImages;
	std::vector<VkImageView>  mSwapChainImageViews;
	VkExtent2D				  mSwapChainExtent;
	VkFormat				  mSwapChainImageFormat;
	FrameData				  mFrameData[FRAME_OVERLAP];
	VkQueue					  mQueue			{ nullptr	};
	uint 					  mQueueFamilyIndex { 0			};
	std::vector<VkSemaphore>  mSignalSemaphores;
	uint					  mSwapChainImageCount;
};