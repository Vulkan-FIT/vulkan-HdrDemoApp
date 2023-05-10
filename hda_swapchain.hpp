#pragma once
#include "hda_instancegpu.hpp"

class HdaSwapchain {

public:

	HdaSwapchain(HdaInstanceGpu& deivce, uint32_t, uint32_t);
	~HdaSwapchain();

	void initSwapchain();
	void cleanupSwapchain();
	vk::ImageView createImageView(vk::Image, vk::Format, vk::ImageAspectFlags, uint32_t, vk::ImageViewType);
	vk::Image createImage(uint32_t, uint32_t, vk::Format, vk::ImageTiling, vk::ImageUsageFlags, vk::MemoryPropertyFlags, vk::DeviceMemory&, uint32_t, vk::ImageCreateFlagBits);

	inline vk::SwapchainKHR getSwapchain() { return swapchain; }
	inline vector<vk::Framebuffer> getFramebuffers() { return framebuffers; }
	inline vk::Extent2D getSurfaceExtent() { return surfaceExtent; }

private:

	void createSwapchain();
	void createSwapchainImageViews();
	void createFramebuffers();
	void createDepthAttachment();

	// TODO TODO smazat
	//vk::ImageView createImageView(vk::Image, vk::Format, vk::ImageAspectFlags);
	//vk::Image createImage(uint32_t, uint32_t, vk::Format, vk::ImageTiling, vk::ImageUsageFlags, vk::MemoryPropertyFlags, vk::DeviceMemory&);

	HdaInstanceGpu& device;
	uint32_t width;
	uint32_t height;

	int eCh = 0;

	vk::PresentModeKHR presentMode{};
	vk::SurfaceCapabilitiesKHR capabilities;
	vk::Extent2D surfaceExtent = vk::Extent2D(0, 0);

	vk::SwapchainKHR swapchain = vk::SwapchainKHR(nullptr);
	vector<vk::Image> swapchainImages{};
	vector<vk::ImageView> swapchainImageViews{};
	vector<vk::Framebuffer> framebuffers{};

	vk::Image depthImage;
	vk::ImageView depthImageView;
	vk::Format depthFormat{};
	vk::DeviceMemory depthImageMem;
};