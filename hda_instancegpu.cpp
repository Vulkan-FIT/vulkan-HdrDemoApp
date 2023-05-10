#include "hda_instancegpu.hpp"

#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>
#include <vector>

using namespace std;

HdaInstanceGpu::HdaInstanceGpu(HdaWindow& window) : window{window} {
	
	cout << "HdaInstanceGpu(): constructor\n";
}

HdaInstanceGpu::~HdaInstanceGpu() {

	cout << "HdaInstaceGpu: Destructor\n";

	if (eCh == 1)
		device.destroy(renderpass);
	device.destroy();
	instance.destroy(winSurface);
	instance.destroy();
}


void HdaInstanceGpu::initGpu() {

	instanceInit();
	createWinSurface();
	findPhysDevice();
	deviceInit();
	renderpassInit();
}


void HdaInstanceGpu::instanceInit() {

	// Vulkan version
	// get function pointer (dynamic)
	auto vkEnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
	if (vkEnumerateInstanceVersion) {
		uint32_t vkversion;
		vkEnumerateInstanceVersion(&vkversion); // in vkEnumerateInstanceVersion is pointer to function, so we can use it this way
		cout << "[INFO]: Vulkan instance version: " << VK_VERSION_MAJOR(vkversion) << "." << VK_VERSION_MINOR(vkversion) << "." << VK_VERSION_PATCH(vkversion) << endl;
	}
	else
		cout << "[INFO]: Vulkan instance version: 1.0\n";

	std::cout << "instanceInit(): Initialization started.\n";

	// get glfw extensions for create vulkan instance 
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	checkExtensionSupport(glfwExtensions, glfwExtensionCount);

	// Vulkan instance
	instance =
		vk::createInstance(
			vk::InstanceCreateInfo{
				vk::InstanceCreateFlags(),  // flags
				&(const vk::ApplicationInfo&)vk::ApplicationInfo{
					"HGDA - HDR Demo App",   // application name
					VK_MAKE_VERSION(1,0,0),			// application version
					nullptr,						// engine name
					VK_MAKE_VERSION(1,0,0),			// engine version
					VK_API_VERSION_1_0,				// api version
				},
				0,        // enabled layer count
				nullptr,  // enabled layer names
				glfwExtensionCount,        // enabled extension count
				glfwExtensions,  // enabled extension names
			}
	);	

	std::cout << "instanceInit(): Initialization end.\n";
}

void HdaInstanceGpu::findPhysDevice() {

	cout << "findPhysDevice(): Enumerate physical devices and check which is suitable for program requirements started.\n";

	vector<vk::PhysicalDevice> devList = instance.enumeratePhysicalDevices();

	// if no device is found that supports Vulkan, there is no point in continuing
	if (devList.empty()) {
		throw std::exception("Not found any Vulkan supported physical devices!\n");
	}

	uint32_t tmpcheck = 0;
	// iterating trough phys devices in device list and check which is suitable for program requirements
	cout << "Available physical devices:\n";
	for (vk::PhysicalDevice physdev : devList) {

		if (isSuitable(physdev)) {
			cout << "isSuitable(): Device rating.\n";
			/*if (physdev.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
				get<3>(suitablePhysDevices.back()) = 100; // score +100
			}
			else if (physdev.getProperties().deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
				get<3>(suitablePhysDevices.back()) = 50; // score +50
			}*/

			// print surface formats
			cout << "Surface formats:" << endl;
			vector<vk::SurfaceFormatKHR> availableSurfaceFormats = physdev.getSurfaceFormatsKHR(winSurface);
			if (availableSurfaceFormats.size() == 0)
				throw std::runtime_error("The list of surface formats is empty.\n");

			for (vk::SurfaceFormatKHR sf : availableSurfaceFormats) {
				cout << vk::to_string(sf.format) << endl;
				if (sf.format == vk::Format::eR16G16B16A16Sfloat && sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
					get<4>(suitablePhysDevices.back()) = sf;
					get<3>(suitablePhysDevices.back()) = 100; // score +100
					break;
				}
				else if (sf.format == vk::Format::eA2B10G10R10UnormPack32 && sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
						get<4>(suitablePhysDevices.back()) = sf;
						get<3>(suitablePhysDevices.back()) = 50; // score +100
						tmpcheck = 1;
				}
				else if (sf.format == vk::Format::eB8G8R8A8Srgb && sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
					if (tmpcheck != 1) {
						get<4>(suitablePhysDevices.back()) = sf;
						tmpcheck = 1;
					}
				}
			}

			/*for (vk::SurfaceFormatKHR sf : availableSurfaceFormats) {
				//cout << vk::to_string(sf.format) << endl;
				if (sf.format == vk::Format::eB8G8R8A8Srgb && sf.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
					get<4>(suitablePhysDevices.back()) = sf;
					get<3>(suitablePhysDevices.back()) = 250; // score +100
				}
			}*/

		}
	}

	if (suitablePhysDevices.empty())
		throw std::runtime_error("Not found any supported physical devices!\n");
	

	uint32_t tmpScore = 0;
	for (const auto& spd : suitablePhysDevices) {

		// TODO TODO defaultni prirazeni prvniho dostupneho formatu ktery se nasel
		//if (get<3>(spd) == 0 && tmpcheck)

		if (get<3>(spd) > tmpScore) {
			tmpScore = get<3>(spd);
			physDevice = get<0>(spd);
			presentationQueueFamily = get<1>(spd);
			graphicsQueueFamily = get<2>(spd);
			
			surfaceFormat = get<4>(spd);
		}
	}

	// HADRCODED SURFACE FORMAT - KEEP COMMENTED
	//surfaceFormat.format = vk::Format::eB8G8R8A8Srgb;
	//surfaceFormat.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

	cout << "Selected physical device: " << physDevice.getProperties().deviceName << std::endl;
	cout << "Selected surfaceFormat:" << vk::to_string(surfaceFormat.format) << " / " << vk::to_string(surfaceFormat.colorSpace) << endl;

	cout << "findPhysDevice(): Function done.\n";
}

void HdaInstanceGpu::createWinSurface() { 
	
	window.createWindowSurface(instance, &winSurface);
	
}

void HdaInstanceGpu::checkExtensionSupport(const char** glfwExts, uint32_t exGlfwCount) {

	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data());

	// TODO TODO smazat vypis
	//std::cout << "available extensions:" << std::endl;
	std::unordered_set<std::string> available;
	for (const auto& extension : extensions) {
		std::cout << "\t" << extension.extensionName << std::endl;
		available.insert(extension.extensionName);
	}

	// creates a vector that reconstructs the vector according to the specified range
	std::vector<const char*> glfwExtensions(glfwExts, glfwExts + exGlfwCount);

	// TODO TODO smazat vypis
	//std::cout << "required extensions:" << std::endl;
	for (const auto& required : glfwExtensions) {
		//std::cout << "\t" << required << std::endl;
		if (available.find(required) == available.end()) {
			throw std::runtime_error("The required glfw extension is missing.\n");
		}
	}
}

bool HdaInstanceGpu::isSuitable(vk::PhysicalDevice physdev) {

	bool swapchainExtCheck = false;

	// check for swapchain extension
	auto extList = physdev.enumerateDeviceExtensionProperties();
	for (vk::ExtensionProperties& extProp : extList) {
		if (strcmp(extProp.extensionName, "VK_KHR_swapchain") == 0) {
			swapchainExtCheck = true;
		}
	}
	if (!swapchainExtCheck)
		return false;

	if (!physdev.getFeatures().samplerAnisotropy)
		return false;

	vk::SurfaceFormatKHR surformat;
	uint32_t graphicsFamily = UINT32_MAX;
	uint32_t presentationFamily = UINT32_MAX;
	vector<vk::QueueFamilyProperties> queueFamilyList = physdev.getQueueFamilyProperties();

	for (uint32_t index = 0, listsize = uint32_t(queueFamilyList.size()); index < listsize; index++) {
	
		// presentation support
		if (physdev.getSurfaceSupportKHR(index, winSurface)) {

			// graphics queue
			if (queueFamilyList[index].queueFlags & vk::QueueFlagBits::eGraphics) {
				suitablePhysDevices.emplace_back(physdev, index, index, 10, surformat);
				return true;
			}
			else {
				// presentation queue
				if (presentationFamily == UINT32_MAX) {
					presentationFamily = index;
				}
			}
		}
		else {
			if (queueFamilyList[index].queueFlags & vk::QueueFlagBits::eGraphics)
				if (graphicsFamily == UINT32_MAX) {
					graphicsFamily = index;
				}
		}

		if (graphicsFamily != UINT32_MAX && presentationFamily != UINT32_MAX) {
			suitablePhysDevices.emplace_back(physdev, presentationFamily, graphicsFamily, 10, surformat);
			return true;
		}
	}

	return false;
}

void HdaInstanceGpu::deviceInit() {

	cout << "deviceInit(): Logical device initialization started.\n";

	array<const char*, 1> devExt = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	vk::PhysicalDeviceFeatures devFeatures{};
	devFeatures.samplerAnisotropy = VK_TRUE;

	// create device
	device =
		physDevice.createDevice(
			vk::DeviceCreateInfo{
			   vk::DeviceCreateFlags(),  // flags
			   graphicsQueueFamily == presentationQueueFamily ? uint32_t(1) : uint32_t(2), // queueCreateInfoCount
			   array{                    // pQueueCreateInfos
				  vk::DeviceQueueCreateInfo{
					 vk::DeviceQueueCreateFlags(),  // flags
					 graphicsQueueFamily,  // queueFamilyIndex
					 1,                    // queueCount
					 &(const float&)1.f,   // pQueuePriorities
				  },
				  vk::DeviceQueueCreateInfo{
					 vk::DeviceQueueCreateFlags(),
					 presentationQueueFamily,
					 1,
					 &(const float&)1.f,
				  },
			   }.data(),
			   0, nullptr,  // no layers
			   1, devExt.data(),  // number of enabled extensions, enabled extension names
			   &devFeatures    // enabled features
			}
	);

	cout << "deviceInit(): Logical device initialization end.\n";

	// get queues - graphicsQueueFamily is index into choosen family
	graphicsQueue = device.getQueue(graphicsQueueFamily, 0);
	presentationQueue = device.getQueue(presentationQueueFamily, 0);
}

//////////////////////////////////////////////////////
vk::Format HdaInstanceGpu::findFormat(vk::ImageTiling tiling) {

	vector<vk::Format> requiredFormats = { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint };

	for (vk::Format format : requiredFormats) {
		vk::FormatProperties props;
		physDevice.getFormatProperties(format, &props);

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
			return format;
		}
	}
	throw std::runtime_error("Required format not found!\n");
}


void HdaInstanceGpu::renderpassInit() {
	
	eCh = 1;
	// render pass
	renderpass =
		device.createRenderPass(
			vk::RenderPassCreateInfo(
				vk::RenderPassCreateFlags(),  // flags
				2,      // attachmentCount
				array{  // pAttachments
					vk::AttachmentDescription(
						vk::AttachmentDescriptionFlags(),  // flags
						surfaceFormat.format,              // format
						vk::SampleCountFlagBits::e1,       // samples
						vk::AttachmentLoadOp::eClear,      // loadOp
						vk::AttachmentStoreOp::eStore,     // storeOp
						vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
						vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
						vk::ImageLayout::eUndefined,       // initialLayout
						vk::ImageLayout::ePresentSrcKHR    // finalLayout
					),
					vk::AttachmentDescription(
						vk::AttachmentDescriptionFlags(),  // flags
						findFormat(vk::ImageTiling::eOptimal),              // format
						vk::SampleCountFlagBits::e1,       // samples
						vk::AttachmentLoadOp::eClear,      // loadOp
						vk::AttachmentStoreOp::eDontCare,     // storeOp
						vk::AttachmentLoadOp::eDontCare,   // stencilLoadOp
						vk::AttachmentStoreOp::eDontCare,  // stencilStoreOp
						vk::ImageLayout::eUndefined,       // initialLayout
						vk::ImageLayout::eDepthStencilAttachmentOptimal    // finalLayout
					),
				}.data(),
				1,      // subpassCount
				array{  // pSubpasses
					vk::SubpassDescription(
						vk::SubpassDescriptionFlags(),     // flags
						vk::PipelineBindPoint::eGraphics,  // pipelineBindPoint
						0,        // inputAttachmentCount
						nullptr,  // pInputAttachments
						1,        // colorAttachmentCount
						array{    // pColorAttachments
							vk::AttachmentReference(
								0,  // attachment
								vk::ImageLayout::eColorAttachmentOptimal  // layout
							),
						}.data(),
						nullptr,  // pResolveAttachments
						array{
							vk::AttachmentReference(
								1,  // attachment
								vk::ImageLayout::eDepthStencilAttachmentOptimal  // layout
							),
						}.data(),  // pDepthStencilAttachment
						0,        // preserveAttachmentCount
						nullptr   // pPreserveAttachments
					),
				}.data(),
				1,      // dependencyCount
				array{  // pDependencies
					vk::SubpassDependency(
						VK_SUBPASS_EXTERNAL,   // srcSubpass
						0,                     // dstSubpass
						vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests),  // srcStageMask
						vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests),  // dstStageMask
						vk::AccessFlags(),     // srcAccessMask
						vk::AccessFlags(vk::AccessFlagBits::eColorAttachmentWrite | // vk::AccessFlagBits::eColorAttachmentRead | 
										vk::AccessFlagBits::eDepthStencilAttachmentWrite),  // dstAccessMask
						vk::DependencyFlags()  // dependencyFlags
					),
				}.data()
			)
		);

	cout << "renderpassInit(): Renderpass is created.\n";
}