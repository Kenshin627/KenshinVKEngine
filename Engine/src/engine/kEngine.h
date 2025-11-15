#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include "typedef.h"
#include "type.h"

constexpr static int FRAME_OVERLAP = 2;

struct FrameData
{
	VkCommandPool	commandPool;
	VkCommandBuffer commandBuffer;
	VkFence		    vkFence;
	VkSemaphore		swapchainSemaphore;
	DeletionQueue	deletionQueue;
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
	void initImmediateCommand();
	void initSyncStructures();
	void initDescriptorSetLayout();
	void initDescriptorSet();
	void initPipeline();
	void initComputePipeline();
	void initGraphicPipeline();
	FrameData& currentFrame();
	void drawBackground();
	void drawGeometry();
	void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);
	void loadMeshBuffer(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	void initDefaultData();
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

	//vma
	VmaAllocator			  mMemAllocator{ nullptr };
	AllocatedImage			  mDrawImage;
	DeletionQueue			  mMainDeletionQueue;


	//descriptorSetlayout & set & pool
	VkDescriptorPool		  mDescriptorPool{ nullptr };	
	VkDescriptorSetLayout     mComputeDescriptorSetLayout{ nullptr };
	VkDescriptorSet			  mComputeDescriptorSet{ nullptr };

	//compute pipeline
	VkPipelineLayout		  mComputePipelineLayout{ nullptr };
	VkPipeline				  mComputePipeline{ nullptr };

	//graphic pipeline
	VkPipelineLayout		  mGraphicPipelineLayout;
	VkPipeline				  mGraphicPipeline;

	//MeshBuffer
	MeshBuffer				  mMeshBuffer;

	VkCommandPool			  mImmediateSubmitPool{ nullptr };
	VkCommandBuffer			  mImmediateSubmitCmd{ nullptr };
	VkFence					  mImmediateSubmitFence{ nullptr };
};