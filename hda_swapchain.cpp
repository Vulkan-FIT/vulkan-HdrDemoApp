#include "hda_swapchain.hpp"

using namespace std;


/*
*
* Most of the initialization features are largely inspired by freely available tutorials
* for understanding the basic principles of working with Vulkan objects.
* 
* https://vkguide.dev
* https://vulkan-tutorial.com/Introduction
* https://www.root.cz/serialy/tutorial-vulkan/
* https://github.com/blurrypiano/littleVulkanEngine/tree/tut27
* https://github.com/amengede/getIntoGameDev/tree/main/vulkan
* https://kohiengine.com
*/

HdaSwapchain::HdaSwapchain(HdaInstanceGpu& device, uint32_t w, uint32_t h) : device{device}, width { w },  height { h } {

	//cout << "HdaSwapchain(): constructor\n";
	cout << ". ";
	//initSwapchain();
}

HdaSwapchain::~HdaSwapchain() {

	cout << "HdaSwapchain: Destructor\n";

	if (eCh == 1)
		cleanupSwapchain();
}

void HdaSwapchain::initSwapchain() {

	eCh = 1;

	createSwapchain();
	createSwapchainImageViews();
	createDepthAttachment();
	createFramebuffers();
}

void HdaSwapchain::cleanupSwapchain() {

	// the order matters
	for (int i = 0; i < framebuffers.size(); i++) { device.getDevice().destroy(framebuffers[i]); }
	for (int i = 0; i < swapchainImageViews.size(); i++) { device.getDevice().destroy(swapchainImageViews[i]); }
	device.getDevice().destroy(depthImageView);
	device.getDevice().destroy(depthImage);
	device.getDevice().freeMemory(depthImageMem);
	device.getDevice().destroy(swapchain);
}

void HdaSwapchain::createSwapchain() {
	
	swapchainImages.clear();
	swapchainImageViews.clear();
	framebuffers.clear();

	// setting the presentation mode
	vector<vk::PresentModeKHR> presentModes = device.getPhysDevice().getSurfacePresentModesKHR(device.getWinSurface());
	presentMode = presentModes[0]; //vk::PresentModeKHR::eFifo; // Immediate max frames
	// finding specific required present mode
	for (const auto& apm : presentModes) { // mailbox na mojem PC neni podporovany
		if (apm == vk::PresentModeKHR::eFifoRelaxed) {
			presentMode = apm;
			break;
		}
	}
	cout << "\nSelected present mode: " << vk::to_string(presentMode) << endl;

	// setting the surface resolution exactly to the window resolution
	capabilities = device.getPhysDevice().getSurfaceCapabilitiesKHR(device.getWinSurface());
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		surfaceExtent = capabilities.currentExtent;
	}
	else {
		surfaceExtent.width = clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		surfaceExtent.height = clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}

	cout << "swapchainInit(): extent: " << surfaceExtent.width << "x" << surfaceExtent.height
		<< ", extent by surfaceCapabilities: " << capabilities.currentExtent.width << "x"
		<< capabilities.currentExtent.height << ", minImageCount: " << capabilities.minImageCount
		<< ", maxImageCount: " << capabilities.maxImageCount << endl;

	// setting image count - for double buffering requiared 2
	uint32_t imageCount = 2;
	if (capabilities.maxImageCount != 0)
		imageCount = clamp(imageCount, capabilities.minImageCount, capabilities.maxImageCount);
	else if (capabilities.minImageCount > 2)
		imageCount = capabilities.minImageCount;


	vk::SwapchainKHR newSwapchain =
		device.getDevice().createSwapchainKHR(
			vk::SwapchainCreateInfoKHR(
				vk::SwapchainCreateFlagsKHR(),  // flags
				device.getWinSurface(),               // surface
				imageCount,
				device.getSurfaceFormat().format,           // imageFormat
				device.getSurfaceFormat().colorSpace,       // imageColorSpace
				surfaceExtent,                  // imageExtent
				1,                              // imageArrayLayers
				vk::ImageUsageFlagBits::eColorAttachment,  // imageUsage
				(device.getGraphicsQueueFamily() == device.getPresentQueueFamily()) ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent, // imageSharingMode
				uint32_t(2),  // queueFamilyIndexCount
				array<uint32_t, 2>{device.getGraphicsQueueFamily(), device.getPresentQueueFamily()}.data(),  // pQueueFamilyIndices
				capabilities.currentTransform,    // preTransform
				vk::CompositeAlphaFlagBitsKHR::eOpaque,  // compositeAlpha
				presentMode,  // presentMode
				VK_TRUE  // clipped
				//swapchain  // oldSwapchain
			)
		);
	cout << "createSwapchain(): Swapchain is created.\n";
	swapchain = newSwapchain;


}

void HdaSwapchain::createSwapchainImageViews() {

	swapchainImages = device.getDevice().getSwapchainImagesKHR(swapchain);
	swapchainImageViews.reserve(swapchainImages.size());

	for (vk::Image image : swapchainImages) {
		swapchainImageViews.emplace_back(createImageView(image, device.getSurfaceFormat().format, vk::ImageAspectFlagBits::eColor, static_cast<uint32_t>(1), vk::ImageViewType::e2D));
	}

	cout << "createImageViews(): Image views are created.\n";
}

void HdaSwapchain::createFramebuffers() {

	framebuffers.reserve(swapchainImages.size()); // + 1 depthImageView
	
	for (size_t i = 0, c = swapchainImages.size(); i < c; i++) {
		std::array<vk::ImageView, 2> imageViews = {	// kdyztak array <x, 2>
				swapchainImageViews[i],
				depthImageView
		};

		framebuffers.emplace_back(
			device.getDevice().createFramebuffer(
				vk::FramebufferCreateInfo(
					vk::FramebufferCreateFlags(),  // flags
					device.getRenderpass(),  // renderPass
					static_cast<uint32_t>(imageViews.size()),  // attachmentCount
					imageViews.data(), //&swapchainImageViews[i],  // pAttachments
					surfaceExtent.width,  // width
					surfaceExtent.height,  // height
					1  // layers
				)
			)
		);
	}
	cout << "createFramebuffers(): Framebuffers are created.\n";
}


vk::ImageView HdaSwapchain::createImageView(vk::Image img, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t layerCount, vk::ImageViewType viewType) {

	vk::ImageView imgView =
		device.getDevice().createImageView(
			vk::ImageViewCreateInfo(
				vk::ImageViewCreateFlags(),  // flags
				img,                       // image
				viewType, // vk::ImageViewType::e2D,      // viewType
				format,        // format
				vk::ComponentMapping(),      // components
				vk::ImageSubresourceRange(   // subresourceRange
					aspectMask,  // aspectMask
					0,  // baseMipLevel
					1,  // levelCount
					0,  // baseArrayLayer
					layerCount //1   // layerCount
				)
			)
		);

	return imgView;
}


vk::Image HdaSwapchain::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags props,
					  vk::DeviceMemory& imgMemory, uint32_t arrLayers, vk::ImageCreateFlagBits flags) {

	vk::Image image =
		device.getDevice().createImage(
			vk::ImageCreateInfo(
				flags & vk::ImageCreateFlagBits::e2DArrayCompatible ? vk::ImageCreateFlags() : flags, //vk::ImageCreateFlags(),
				vk::ImageType::e2D,
				format,
				vk::Extent3D(width, height, 1),
				uint32_t(1),
				arrLayers, //uint32_t(1),
				vk::SampleCountFlagBits::e1,
				tiling,
				usage,
				vk::SharingMode::eExclusive,
				{},
				{},
				vk::ImageLayout::eUndefined
			)
		);


	vk::MemoryRequirements memRequirements = device.getDevice().getImageMemoryRequirements(image);

	vk::PhysicalDeviceMemoryProperties memProperties = device.getPhysDevice().getMemoryProperties();
	uint32_t memoryTypeIndex, check = UINT32_MAX;
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if (check != 1) {
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
				(props)) == (props)) {
				memoryTypeIndex = i;
				check = 1;
			}
		}
	}
	if (check == UINT32_MAX) {
		throw runtime_error("Corresponding memory type not found.\n");
	}

	imgMemory =
		device.getDevice().allocateMemory(
			vk::MemoryAllocateInfo(
				memRequirements.size,
				memoryTypeIndex
			)
		);
	device.getDevice().bindImageMemory(image, imgMemory, 0);

	return image;
}

void HdaSwapchain::createDepthAttachment() {

	depthFormat = device.getFindFormatFunc(vk::ImageTiling::eOptimal);
	depthImage = createImage(surfaceExtent.width, surfaceExtent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
							 vk::MemoryPropertyFlagBits::eDeviceLocal, depthImageMem, static_cast<uint32_t>(1), vk::ImageCreateFlagBits::e2DArrayCompatible);

	depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, static_cast<uint32_t>(1), vk::ImageViewType::e2D);

	cout << "createDepthAttachment(): Depth attachment is created.\n";
}