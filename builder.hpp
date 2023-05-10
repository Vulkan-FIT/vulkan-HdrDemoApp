#pragma once
#include "hda_instancegpu.hpp"
#include "hda_window.hpp"
#include "hda_swapchain.hpp"
#include "hda_pipeline.hpp"
#include "hda_sceneobject.hpp"

//#include "vk_mem_alloc.h"		TODO TODO DELETE
#include <chrono>

#define OBJECTS_NUMBER 3
#define PARALLEL_FRAMES 2
#define SELECT_SCENEDATA 1
#define SELECT_MATLIGHTDATA 2
#define NUM_OF_PARTS_WITH_SAME_TEX 103


/*
*
* The core of the entire application, which assembles 
* the individual parts and classes into a single unit 
* and performs the rendering process.
*
*/

class HdaBuilder {

public:

	HdaBuilder(HdaInstanceGpu&, HdaWindow&, HdaSwapchain&, HdaPipeline&);
	~HdaBuilder();

	void initBuilder();
	void render();

private:

	HdaInstanceGpu& device;
	HdaWindow& window;
	HdaSwapchain& swapchain;
	HdaPipeline& pipeline;

	//MT

	void createCommandPool();
	void createCommandBuffer();
	void createSemaphores();
	void createFences();

	void recreateSwapchain();
	void initSyncObjects();
	void cleanupSyncObjects();

	void createBuffer(vk::DeviceSize, vk::BufferUsageFlags, vk::MemoryPropertyFlags, vk::Buffer&, vk::DeviceMemory&);
	void copyBuffers(vk::Buffer, vk::Buffer, vk::DeviceSize);

	inline size_t calcRequiredAligment(size_t);
	void createVertexBuffer(HdaModel::Mesh&);
	void createIndexBuffer(HdaModel::Mesh&);

	void createUniformBuffers();
	void createDynamicUniformBuffer();	// DELETE

	void createDescriptorPool();
	void HdaBuilder::updateDescrSets(vk::DescriptorSet, const vk::DescriptorImageInfo*);
	vector<vk::DescriptorSet> createDescriptorSets(vk::DescriptorSetLayout, const vk::DescriptorImageInfo*, uint32_t, uint32_t, uint32_t);

	void loadMesh(HdaModel::Mesh&, const char*, string);
	void loadTexture(HdaModel::Texture& , const char*);

	void layoutConversion(vk::Image, vk::Format, vk::ImageLayout, vk::ImageLayout, array<vk::AccessFlagBits, 2>, array<vk::PipelineStageFlagBits, 2>, uint32_t, uint32_t);

	void copyBuffToImage(vk::Buffer, vk::Image, vk::ImageLayout, uint32_t, uint32_t, uint32_t, uint32_t);

	void loadTextureCubemap(vector<const char*>, HdaModel::Texture&);
	void loadCubemapSkybox(HdaModel::SceneObject*);
	void cleanupSceneObjects(vector<HdaModel::SceneObject>);

	void mapMemoryToUniformBuffer(vk::DeviceMemory, size_t, uint32_t, HdaModel::MaterialLightUniformData, uint32_t, HdaModel::SceneUniformData, uint32_t);

	void calculateAdditionalData();
	void calculateLightColor(glm::vec4 lightC, glm::vec4 diffuse, glm::vec4 ambient);

	void loadScene();
	void setUniformStructures(HdaModel::SceneObject*, vk::CommandBuffer*, float, uint32_t, uint32_t, HdaModel::MaterialLightUniformData*, HdaModel::SceneUniformData*);
	void drawMultiTexturedObjects(HdaModel::SceneObject*, vk::CommandBuffer*, float);
	void drawSingleTexturedObjects(HdaModel::SceneObject* o, vk::CommandBuffer* cmdBuffs, float t, int i);
	void drawScene(vk::CommandBuffer*);


	inline void fps();
	inline void p(string str) { cout << str << endl; };

	int actual_frame = 0;
	int ch = 0;

	vector<vk::CommandPool> commandPools;
	vector<vk::CommandBuffer> commandBuffers;
	vector<vk::Semaphore> presentCompleteSemaphores;
	vector<vk::Semaphore> renderCompleteSemaphores;
	vector<vk::Fence> renderCompleteFences;

	std::vector<vk::Buffer> uniformBuffs;
	std::vector<vk::DeviceMemory> uniformBuffsMemory;
	std::vector<void*> uniformBuffsMemoryPointer;

	vk::Buffer sceneUniformBuff;
	vk::DeviceMemory sceneUniformBuffMemory;

	vk::Buffer materialLightUniformBuff;
	vk::DeviceMemory materialLightUniformBuffMemory;

	// // /// // // // // // //	TODO TODO DELETE
	//VmaAllocator allocator;
	//VkBuffer dynamicBuff;
	//VmaAllocation allocation;
	// /// /// // / /// /// ///

	vk::DescriptorPool descriptorPool;
	vector<vk::DescriptorSet> descriptorSets;

	// TODO TODO smazat nontexturedobjects
	vector<HdaModel::SceneObject> sceneObjects;
	vector<HdaModel::SceneObject> sceneNontexturedObjects;

	uint32_t requiredAlignmentScene{};
	uint32_t requiredAlignmentMaterial{};
	size_t sceneUniformSize{};
	size_t materialUniformSize{};
	glm::vec4 lightColor{};
	glm::vec4 diffuseColor{};
	glm::vec4 ambientColor{};
	uint32_t sceneObjectsSize = UINT32_MAX;

	int hdrOnFlag = 0;
	float exposure = 1.0f;
	int chooseMethodFlag = 0;

	double currentT = 0, diff = 0, frameRate = 0, frames = 0.0, lastT = 0.0;
	chrono::high_resolution_clock::time_point cT;
	chrono::high_resolution_clock::time_point lT;

};