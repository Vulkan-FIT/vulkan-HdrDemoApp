#pragma once
#include "hda_window.hpp"
#include "hda_model.hpp"


/*
*
* A class representing a logical device.
*
*/

using namespace std;

class HdaInstanceGpu { //device instance

public:

	HdaInstanceGpu(HdaWindow& window);
	~HdaInstanceGpu();

	void initGpu();

	inline vk::Instance getInstance() { return instance; }
	inline vk::Device getDevice() { return device; }
	inline vk::PhysicalDevice getPhysDevice() { return physDevice; }

	inline vk::SurfaceKHR getWinSurface() { return winSurface; }
	inline vk::SurfaceFormatKHR getSurfaceFormat() { return surfaceFormat; }

	inline uint32_t getGraphicsQueueFamily() { return graphicsQueueFamily; }
	inline uint32_t getPresentQueueFamily() { return presentationQueueFamily; }

	inline vk::Format getFindFormatFunc(vk::ImageTiling t) { return findFormat(t); }
	inline vk::RenderPass getRenderpass() { return renderpass;  }

	inline vk::Queue getGraphicsQueue() { return graphicsQueue; }
	inline vk::Queue getPresentationQueue() { return presentationQueue; }


private:


	void instanceInit();
	void findPhysDevice();
	bool isSuitable(vk::PhysicalDevice physdev);
	void deviceInit();
	void createWinSurface();
	void checkExtensionSupport(const char **, uint32_t);
	vk::Format findFormat(vk::ImageTiling);
	void renderpassInit();

	int eCh = 0;
	
	HdaWindow& window;
	vk::SurfaceKHR winSurface;

	vk::RenderPass renderpass;

	vk::Instance instance;
	vk::Device device;
	vector<tuple<vk::PhysicalDevice, uint32_t, uint32_t, uint32_t, vk::SurfaceFormatKHR>> suitablePhysDevices{};
	vk::PhysicalDevice physDevice;
	uint32_t graphicsQueueFamily = UINT32_MAX;
	uint32_t presentationQueueFamily = UINT32_MAX;
	vk::Queue graphicsQueue;
	vk::Queue presentationQueue;

	vk::SurfaceFormatKHR surfaceFormat;
};

