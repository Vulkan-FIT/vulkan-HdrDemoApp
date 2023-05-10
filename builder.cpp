#include "builder.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "external/include/stb_image.h"


/*
*
* Standard Vulkan object initializations are largely inspired
* by freely available tutorials.
*
* https://vkguide.dev
* https://vulkan-tutorial.com/Introduction
* https://www.root.cz/serialy/tutorial-vulkan/
* https://github.com/blurrypiano/littleVulkanEngine/tree/tut27
* https://github.com/amengede/getIntoGameDev/tree/main/vulkan
* https://kohiengine.com
*/

HdaBuilder::HdaBuilder(HdaInstanceGpu& dev, HdaWindow& win, HdaSwapchain& swa, HdaPipeline& pip) : device{ dev }, window{ win }, swapchain{ swa }, pipeline{ pip } {
	
	cout << "HdaBuilder(): constructor\n";

}

HdaBuilder::~HdaBuilder() {

	cout << "HdaBuiler: Destructor\n";

	if (ch == 1) {

		device.getGraphicsQueue().waitIdle();

		cleanupSyncObjects();

		if (!commandPools.empty() && !commandBuffers.empty())
			for (int i = 0; i < PARALLEL_FRAMES; i++) {
				device.getDevice().freeCommandBuffers(commandPools[i], commandBuffers[i]);
			}

		for (int i = 0; i < uniformBuffs.size(); i++)
			device.getDevice().destroyBuffer(uniformBuffs[i]);

		for (int i = 0; i < uniformBuffsMemory.size(); i++)
			device.getDevice().freeMemory(uniformBuffsMemory[i]);

		device.getDevice().destroyBuffer(sceneUniformBuff);

		device.getDevice().freeMemory(sceneUniformBuffMemory);
		device.getDevice().destroyBuffer(materialLightUniformBuff);
		device.getDevice().freeMemory(materialLightUniformBuffMemory);

		device.getDevice().destroyDescriptorPool(descriptorPool);

		cleanupSceneObjects(sceneObjects);

		for (int i = 0; i < commandPools.size(); i++)
			device.getDevice().destroyCommandPool(commandPools[i]);

	}
}

void HdaBuilder::initBuilder() {

	ch = 1;

	for (int i = 0; i < OBJECTS_NUMBER; i++) {

		HdaModel::SceneObject obj;
		sceneObjects.push_back(obj);
	}


	createCommandPool();
		
	createUniformBuffers();
	createDynamicUniformBuffer();
	createDescriptorPool();

	createCommandBuffer();
	initSyncObjects();

	loadScene();

	calculateAdditionalData();
}


void HdaBuilder::cleanupSceneObjects(vector<HdaModel::SceneObject> o) {

	for (int k = 0; k < o.size(); k++) {

		for (int i = 0; i < o[k].objectTexture.size(); i++) {
			device.getDevice().destroySampler(o[k].objectTexture[i].textureSampler);
			device.getDevice().destroyImageView(o[k].objectTexture[i].textureImageView);
			device.getDevice().destroyImage(o[k].objectTexture[i].textureImage);
			device.getDevice().freeMemory(o[k].objectTexture[i].textureImageMemory);

		}

		device.getDevice().destroyBuffer(o[k].objectMesh.vertexBuff);
		device.getDevice().freeMemory(o[k].objectMesh.vertexBuffMemory);

		device.getDevice().destroyBuffer(o[k].objectMesh.indexBuff);
		device.getDevice().freeMemory(o[k].objectMesh.indexBuffMemory);

		device.getDevice().destroyDescriptorSetLayout(o[k].objectDescriptSetLay);
		device.getDevice().destroyPipeline(o[k].objectPipeline);
		device.getDevice().destroyPipelineLayout(o[k].objectPipelineLayout);
	}
}


void HdaBuilder::createCommandPool() {

	commandPools.resize(PARALLEL_FRAMES);

	for (int i = 0; i < PARALLEL_FRAMES; i++) {
		commandPools[i] =
			device.getDevice().createCommandPool(
				vk::CommandPoolCreateInfo(
					vk::CommandPoolCreateFlagBits::eTransient |
					vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
					device.getGraphicsQueueFamily()
				)
			);
	}
}


void HdaBuilder::createCommandBuffer() {

	commandBuffers.resize(PARALLEL_FRAMES);

	for (int i = 0; i < PARALLEL_FRAMES; i++) {
		commandBuffers[i] =
			device.getDevice().allocateCommandBuffers(
				vk::CommandBufferAllocateInfo(
					commandPools[i],
					vk::CommandBufferLevel::ePrimary,
					1  
				)
			)[0];
	}
}


void HdaBuilder::createSemaphores() {

	presentCompleteSemaphores.resize(PARALLEL_FRAMES);
	renderCompleteSemaphores.resize(PARALLEL_FRAMES);

	for (int i = 0; i < PARALLEL_FRAMES; i++) {
		presentCompleteSemaphores[i] =
			device.getDevice().createSemaphore(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()
				)
			);
		renderCompleteSemaphores[i] =
			device.getDevice().createSemaphore(
				vk::SemaphoreCreateInfo(
					vk::SemaphoreCreateFlags()
				)
			);
	}
}


void HdaBuilder::createFences() {

	renderCompleteFences.resize(PARALLEL_FRAMES);

	for (int i = 0; i < PARALLEL_FRAMES; i++) {
		renderCompleteFences[i] =
			device.getDevice().createFence(
				vk::FenceCreateInfo(
					vk::FenceCreateFlagBits::eSignaled
				)
			);
	}
}


void HdaBuilder::recreateSwapchain() {

	int width = 0, height = 0;
	glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
	while (width == 0 || height == 0) {
		cout << "Window is not visible, waiting for resize or maximizing\n";
		glfwGetFramebufferSize(window.getGLFWwindow(), &width, &height);
		glfwWaitEvents();
	}

	device.getDevice().waitIdle();

	for (int i = 0; i < sceneObjects.size(); i++) {
		device.getDevice().destroyPipeline(sceneObjects[i].objectPipeline);
	}

	swapchain.cleanupSwapchain();
	cleanupSyncObjects();
	swapchain.initSwapchain();

	for (int i = 0; i < sceneObjects.size()-1; i++)
		sceneObjects[i].objectPipeline = pipeline.createPipeline(vk::PipelineCreateFlags(), UINT32_MAX, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
			sceneObjects[i].objectPipelineLayout, device.getRenderpass(), UINT32_MAX, nullptr, UINT32_MAX);

	// pipeline for skybox requires different parameters
	sceneObjects[sceneObjects.size()-1].objectPipeline = pipeline.createPipeline(vk::PipelineCreateFlags(), UINT32_MAX,
		array{  // pStages
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),
						vk::ShaderStageFlagBits::eVertex,  // stage
						pipeline.getSkyboxVertexShaderModule(),  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					},
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),
						vk::ShaderStageFlagBits::eFragment,  // stage
						pipeline.getSkyboxFragmentShaderModule(),  // module
						"main",  // pName
						nullptr // pSpecializationInfo
					},
		}.data(), nullptr, nullptr, nullptr, nullptr,
		&(const vk::PipelineRasterizationStateCreateInfo&)vk::PipelineRasterizationStateCreateInfo{  // pRasterizationState
					vk::PipelineRasterizationStateCreateFlags(),
					VK_FALSE,  // depthClampEnable
					VK_FALSE,  // rasterizerDiscardEnable
					vk::PolygonMode::eFill,  // polygonMode
					vk::CullModeFlagBits::eFront,  // cullMode PUVODNE eNone
					vk::FrontFace::eCounterClockwise,  // frontFace
					VK_FALSE,  // depthBiasEnable
					0.f,  // depthBiasConstantFactor
					0.f,  // depthBiasClamp
					0.f,  // depthBiasSlopeFactor
					1.f   // lineWidth
		}, nullptr, nullptr, nullptr, nullptr, sceneObjects[sceneObjects.size()-1].objectPipelineLayout, device.getRenderpass(), UINT32_MAX, nullptr, UINT32_MAX);
	
	initSyncObjects();
}


void HdaBuilder::initSyncObjects() {

	createSemaphores();
	createFences();
}

void HdaBuilder::cleanupSyncObjects() {

	if (!renderCompleteFences.empty() && !renderCompleteSemaphores.empty() && !presentCompleteSemaphores.empty())
		for (int i = 0; i < PARALLEL_FRAMES; i++) {
			device.getDevice().destroyFence(renderCompleteFences[i]);
			device.getDevice().destroySemaphore(renderCompleteSemaphores[i]);
			device.getDevice().destroySemaphore(presentCompleteSemaphores[i]);
		}
}


/**
*	@brief Create new vertex buffer.
*
*	Creates a new vertex buffer belonging to the given Mesh object.
*
*	Code modified from tutorial article obtained from Vulkan-tutorial:
*	https://vulkan-tutorial.com/Vertex_buffers/Vertex_buffer_creation
*
*/
void HdaBuilder::createVertexBuffer(HdaModel::Mesh& mesh) {

	vk::Buffer hostBuff;
	vk::DeviceMemory hostBuffMemory;

	// creating buffer with cpu access memory type
	createBuffer((sizeof(mesh.meshVertices[0]) * mesh.meshVertices.size()), vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, hostBuff, hostBuffMemory);

	try {
		void* data = device.getDevice().mapMemory(hostBuffMemory, 0, sizeof(mesh.meshVertices[0]) * mesh.meshVertices.size(), vk::MemoryMapFlags());
		memcpy(data, mesh.meshVertices.data(), (size_t)sizeof(mesh.meshVertices[0]) * mesh.meshVertices.size());
		device.getDevice().unmapMemory(hostBuffMemory);

		// creating vertex buffer in device local memory on gpu
		createBuffer((sizeof(mesh.meshVertices[0]) * mesh.meshVertices.size()), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal, mesh.vertexBuff, mesh.vertexBuffMemory);

		// copying data from hostBuffer to vertexBuffer
		copyBuffers(hostBuff, mesh.vertexBuff, sizeof(mesh.meshVertices[0]) * mesh.meshVertices.size());
	}
	catch (...) {
		device.getDevice().destroyBuffer(hostBuff);
		device.getDevice().freeMemory(hostBuffMemory);
		throw runtime_error("Unspecified error in createVertexBuffer().");
	}
	device.getDevice().destroyBuffer(hostBuff);
	device.getDevice().freeMemory(hostBuffMemory);

	cout << "createVertexBuffer(): Vertex buffer is created.\n";
}


/**
*	@brief Create new index buffer.
*
*	Creates a new index buffer belonging to the given Mesh object.
*
*	Code modified from tutorial article obtained from Vulkan-tutorial:
*	https://vulkan-tutorial.com/Vertex_buffers/Index_buffer
*
*/
void HdaBuilder::createIndexBuffer(HdaModel::Mesh& mesh) {

	vk::Buffer hostBuff;
	vk::DeviceMemory hostBuffMemory;

	// creating buffer with cpu access memory type
	createBuffer((sizeof(mesh.meshIndices[0]) * mesh.meshIndices.size()), vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, hostBuff, hostBuffMemory);
	try {
		void* data = device.getDevice().mapMemory(hostBuffMemory, 0, sizeof(mesh.meshIndices[0]) * mesh.meshIndices.size(), vk::MemoryMapFlags());
		memcpy(data, mesh.meshIndices.data(), (size_t)sizeof(mesh.meshIndices[0]) * mesh.meshIndices.size());
		device.getDevice().unmapMemory(hostBuffMemory);

		// creating vertex buffer in device local memory on gpu
		createBuffer((sizeof(mesh.meshIndices[0]) * mesh.meshIndices.size()), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal, mesh.indexBuff, mesh.indexBuffMemory);

		// copying data from hostBuffer to vertexBuffer
		copyBuffers(hostBuff, mesh.indexBuff, sizeof(mesh.meshIndices[0]) * mesh.meshIndices.size());
	}
	catch (...) {
		device.getDevice().destroyBuffer(hostBuff);
		device.getDevice().freeMemory(hostBuffMemory);
		throw runtime_error("Unspecified error in createIndexBuffer().");
	}
	device.getDevice().destroyBuffer(hostBuff);
	device.getDevice().freeMemory(hostBuffMemory);

	cout << "createIndexBuffer(): Index buffer is created.\n";
}

/**
*	@brief Calculate required alignment.
* 
*	Calculate the alignment for the data to be stored in the dynamic uniform buffer.
* 
*	Code modified from tutorial article obtained from Sascha Willems github:
*	https://github.com/SaschaWillems/Vulkan/blob/master/examples/dynamicuniformbuffer/README.md
*
*/
size_t HdaBuilder::calcRequiredAligment(size_t structureSize) {

	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = device.getPhysDevice().getProperties().limits.minUniformBufferOffsetAlignment;

	size_t alignedSize = structureSize;
	if (minUboAlignment > 0) {
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	return alignedSize;
}


void HdaBuilder::createUniformBuffers() {

	uniformBuffs.resize(PARALLEL_FRAMES);
	uniformBuffsMemory.resize(PARALLEL_FRAMES);
	uniformBuffsMemoryPointer.resize(PARALLEL_FRAMES);
	uint32_t submeshCount = 0;	// every submesh has own texture

	// TODO TODO	NUM_OF_PARTS_WITH_SAME_TEX 
	// DYNAMIC uniform buffers
	createBuffer((NUM_OF_PARTS_WITH_SAME_TEX + sceneObjects.size()) * calcRequiredAligment(sizeof(HdaModel::SceneUniformData)), vk::BufferUsageFlagBits::eUniformBuffer,		// 24
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, sceneUniformBuff, sceneUniformBuffMemory);

	// TODO TODO DELETE
	//for (int32_t i = 0; i < sceneObjects.size(); i++)
		//submeshCount += sceneObjects[i].objectMesh.submeshCnt;
	createBuffer((NUM_OF_PARTS_WITH_SAME_TEX + sceneObjects.size()) * calcRequiredAligment(sizeof(HdaModel::MaterialLightUniformData)), vk::BufferUsageFlagBits::eUniformBuffer,	// 24
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, materialLightUniformBuff, materialLightUniformBuffMemory);
	//P; cout << submeshCount << endl;

	// STATIC uniform buffers
	for (int i = 0; i < PARALLEL_FRAMES; i++) {
		createBuffer(sizeof(HdaModel::ProjectionUniformData), vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffs[i], uniformBuffsMemory[i]);

		uniformBuffsMemoryPointer[i] = 
			device.getDevice().mapMemory(
				uniformBuffsMemory[i], 
				0, 
				sizeof(HdaModel::ProjectionUniformData),
				vk::MemoryMapFlags()
			);
	}
	cout << "createUniformBuffers(): Uniform buffers are created.\n";
}

void HdaBuilder::createDynamicUniformBuffer() {
	
	// TODO TODO DELETE

}


void HdaBuilder::createDescriptorPool() {

	descriptorPool =
		device.getDevice().createDescriptorPool(
			vk::DescriptorPoolCreateInfo(
				vk::DescriptorPoolCreateFlags(),
				static_cast<uint32_t>(PARALLEL_FRAMES * (sceneObjects.size()*106)),
				static_cast < uint32_t>(4),	// CHANGED
				array{ 
					vk::DescriptorPoolSize(
						vk::DescriptorType::eUniformBuffer,
						static_cast<uint32_t>(PARALLEL_FRAMES * (sceneObjects.size() * 106))
					),
					vk::DescriptorPoolSize(
						vk::DescriptorType::eCombinedImageSampler,
						static_cast<uint32_t>(PARALLEL_FRAMES * (sceneObjects.size() * 106))
					),
					vk::DescriptorPoolSize(		// CHANGED
						vk::DescriptorType::eUniformBufferDynamic,
						static_cast<uint32_t>(PARALLEL_FRAMES * (sceneObjects.size() * 106))
					),
					vk::DescriptorPoolSize(		// CHANGED
						vk::DescriptorType::eUniformBufferDynamic,
						static_cast<uint32_t>(PARALLEL_FRAMES * (sceneObjects.size() * 106))
					)
				}.data()
			)
		);
	cout << "createDescriptorPool(): Descriptor pool is created.\n";
}

void HdaBuilder::updateDescrSets(vk::DescriptorSet descrSet, const vk::DescriptorImageInfo* descrImage) {

		device.getDevice().updateDescriptorSets(
			array{
				vk::WriteDescriptorSet(
					descrSet,
					3, //descrBind, //1,
					0, //0,
					1, //1,
					vk::DescriptorType::eCombinedImageSampler,
					descrImage,
					nullptr,
					nullptr
				)
			},
			nullptr
		);
}


vector<vk::DescriptorSet> HdaBuilder::createDescriptorSets(vk::DescriptorSetLayout lay, const vk::DescriptorImageInfo* descrImage, uint32_t aE, uint32_t dC, uint32_t cnt) { 

	vector<vk::DescriptorSet> descrSets;
	descrSets.resize(PARALLEL_FRAMES);

	descrSets =
		device.getDevice().allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo(
				descriptorPool,
				static_cast<uint32_t>(PARALLEL_FRAMES)*cnt,
				vector<vk::DescriptorSetLayout>(PARALLEL_FRAMES, lay).data()
			)
		);

	for (int i = 0; i < PARALLEL_FRAMES; i++) {

		device.getDevice().updateDescriptorSets(
			array{
				vk::WriteDescriptorSet(
					descrSets[i],
					0,
					0,
					1,
					vk::DescriptorType::eUniformBuffer,
					nullptr,
					array{
						vk::DescriptorBufferInfo(
							uniformBuffs[i],
							0,
							sizeof(HdaModel::ProjectionUniformData)
						)
					}.data(),
					nullptr
				),
				vk::WriteDescriptorSet(
					descrSets[i],
					1,
					0,
					1,
					vk::DescriptorType::eUniformBufferDynamic,
					nullptr,
					array{
						vk::DescriptorBufferInfo(
							sceneUniformBuff,	//dynamicBuff,
							0,
							sizeof(HdaModel::SceneUniformData)
						)
					}.data(),
					nullptr
				),
				vk::WriteDescriptorSet(
					descrSets[i],
					2,
					0,
					1,
					vk::DescriptorType::eUniformBufferDynamic,
					nullptr,
					array{
						vk::DescriptorBufferInfo(
							materialLightUniformBuff,	//dynamicBuff,
							0,
							sizeof(HdaModel::MaterialLightUniformData)
						)
					}.data(),
					nullptr
				),
				vk::WriteDescriptorSet(
					descrSets[i],
					3, //descrBind, //1,
					aE, //0,
					dC, //1,
					vk::DescriptorType::eCombinedImageSampler,
					descrImage,
					nullptr,
					nullptr
				)
			},
			nullptr
		);
	}
	//cout << "createDescriptorSets(): Descriptor sets are created.\n";

	return descrSets;
}


/**
*
*	Creates a new vk::Buffer according to the specified parameters.
*
*	Code modified from tutorial article obtained from Vulkan-tutorial:
*	https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
*
*/
void HdaBuilder::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buff, vk::DeviceMemory& buffMemory) {

	buff =
		device.getDevice().createBuffer(
			vk::BufferCreateInfo(
				vk::BufferCreateFlags(),
				size,
				usage,
				vk::SharingMode::eExclusive
			)
		);

	vk::MemoryRequirements memRequirements = device.getDevice().getBufferMemoryRequirements(buff);

	vk::PhysicalDeviceMemoryProperties memProperties = device.getPhysDevice().getMemoryProperties();
	uint32_t memoryTypeIndex, check = UINT32_MAX;
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if (check != 1) {
			if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
				(properties)) == (properties)) {
				memoryTypeIndex = i;
				check = 1;
			}
		}
	}
	if (check == UINT32_MAX) {
		throw runtime_error("Corresponding memory type not found.\n");
	}

	buffMemory =
		device.getDevice().allocateMemory(
			vk::MemoryAllocateInfo(
				memRequirements.size,
				memoryTypeIndex
			)
		);
	device.getDevice().bindBufferMemory(buff, buffMemory, 0);
}


void HdaBuilder::copyBuffers(vk::Buffer srcBuff, vk::Buffer dstBuff, vk::DeviceSize size) {

	vk::CommandBuffer commandBuff =
		device.getDevice().allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				commandPools[actual_frame],
				vk::CommandBufferLevel::ePrimary,
				1
			)
		)[0];

	commandBuff.begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr  // pInheritanceInfo
		)
	);

	commandBuff.copyBuffer(srcBuff, dstBuff, 1, &vk::BufferCopy({0,0, size }));

	commandBuff.end();

	device.getGraphicsQueue().submit(
		vk::ArrayProxy<const vk::SubmitInfo>(
			1,
			&(const vk::SubmitInfo&)vk::SubmitInfo(
				0, nullptr,
				nullptr,
				1, &commandBuff, 
				0, nullptr 
			)
		)
	);

	device.getGraphicsQueue().waitIdle();
	device.getDevice().freeCommandBuffers(commandPools[actual_frame], commandBuff);
}

// TODO TODO 
void HdaBuilder::loadMesh(HdaModel::Mesh& mesh, const char* filename, string mtlBaseDir) {

	mesh.loadObjFormat(filename, mtlBaseDir);
	createVertexBuffer(mesh);
	//createIndexBuffer(mesh);

	cout << "loadMesh(): Mesh" << filename << " is loaded.\n";
}


void HdaBuilder::loadTexture(HdaModel::Texture& texture, const char* filename) {

	int texWidth, texHeight, texChannels;
	//float* pixels{};

	// TODO TODO    PUVODNI
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		throw runtime_error("Failed to load texture file.\n");
	}

	/*if (!(pixels = stbi_loadf(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha))) {
		throw runtime_error("Failed to load cubemap texture file.\n");
	}*/

	//vk::DeviceSize teximageSize = static_cast<uint32_t>(texWidth * texHeight * 4 * 4);
	vk::DeviceSize teximageSize = static_cast<uint32_t>(texWidth * texHeight * 4);

	vk::Buffer hostBuff;
	vk::DeviceMemory hostBuffMemory;

	// creating buffer with cpu access memory type
	createBuffer(teximageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, hostBuff, hostBuffMemory);

	try {

		void* data = device.getDevice().mapMemory(hostBuffMemory, 0, teximageSize, vk::MemoryMapFlags());
		memcpy(data, pixels, static_cast<size_t>(teximageSize));
		device.getDevice().unmapMemory(hostBuffMemory);
		stbi_image_free(pixels);
																		  // vk::Format::eR8G8B8A8Srgb
		texture.textureImage = swapchain.createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal, texture.textureImageMemory, static_cast<uint32_t>(1), vk::ImageCreateFlagBits::e2DArrayCompatible);

		texture.textureImageView = swapchain.createImageView(texture.textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, static_cast<uint32_t>(1), vk::ImageViewType::e2D);

		texture.textureSampler =
			device.getDevice().createSampler(
				vk::SamplerCreateInfo(
					vk::SamplerCreateFlags(),
					vk::Filter::eLinear,
					vk::Filter::eLinear,
					vk::SamplerMipmapMode::eLinear,
					vk::SamplerAddressMode::eRepeat,
					vk::SamplerAddressMode::eRepeat,
					vk::SamplerAddressMode::eRepeat,
					{},
					VK_TRUE,	// nebo false
					device.getPhysDevice().getProperties().limits.maxSamplerAnisotropy, // 1.0f
					VK_FALSE,
					vk::CompareOp::eAlways,
					{},
					{},
					vk::BorderColor::eIntOpaqueBlack,//eIntOpaqueBlack,
					VK_FALSE
				)
			);

		layoutConversion(texture.textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
			{ vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite }, { vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer }, static_cast<uint32_t>(1), static_cast<uint32_t>(0));

		copyBuffToImage(hostBuff, texture.textureImage, vk::ImageLayout::eTransferDstOptimal, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), static_cast<uint32_t>(1), static_cast<uint32_t>(0));
	
	}
	catch (...) {
		stbi_image_free(pixels);
		device.getDevice().destroyBuffer(hostBuff);
		device.getDevice().freeMemory(hostBuffMemory);
		throw runtime_error("Unspecified error in loadTexture().\n");
	}
	
	device.getDevice().destroyBuffer(hostBuff);
	device.getDevice().freeMemory(hostBuffMemory);

	layoutConversion(texture.textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
					{ vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead }, { vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader }, static_cast<uint32_t>(1), static_cast<uint32_t>(0));

	cout << "loadTexture(): Texture " << filename << " is loaded.\n";
}


void HdaBuilder::loadTextureCubemap(vector<const char*> filename, HdaModel::Texture& texture) {
	
	for (size_t i = 0; i < filename.size(); i++) {

		//stbi_uc* tex{nullptr};
		float* tex{ nullptr };
		int texWidth = 0, texHeight = 0, texChannels = 0;
		//int texWidthH = 0, texHeightH = 0, texChannelsH = 0;

		if (!(tex = stbi_loadf(filename[i], &texWidth, &texHeight, &texChannels, STBI_rgb_alpha))) {
			throw runtime_error("Failed to load cubemap texture file.\n");
		}
		// TODO TODO puvodni funkce nacitani textur v celych cislech stbi_load()
		/*if (!(texHdr = stbi_loadf(filename[i], &texWidthH, &texHeightH, &texChannelsH, STBI_rgb_alpha))) {
			throw runtime_error("Failed to load cubemap texture file.\n");
		}*/
		//cout << "texChannels: " << texChannels << endl;
		
		vk::DeviceSize layerSize = static_cast<uint64_t>(texWidth * texHeight * 4 * 2 * 2);
		vk::DeviceSize teximageSize = static_cast<uint64_t>(layerSize * 6);
		vk::Buffer hostBuff;
		vk::DeviceMemory hostBuffMemory;

		if (i == 0) {																// eR8G8B8A8Srgb eR16G16B16A16Sfloat
			texture.textureImage = swapchain.createImage(texWidth, texHeight, vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
				vk::MemoryPropertyFlagBits::eDeviceLocal, texture.textureImageMemory, static_cast<uint32_t>(filename.size()), vk::ImageCreateFlagBits::eCubeCompatible);
		
			texture.textureImageView = swapchain.createImageView(texture.textureImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor, static_cast<uint32_t>(filename.size()), vk::ImageViewType::eCube);

			texture.textureSampler =
				device.getDevice().createSampler(
					vk::SamplerCreateInfo(
						vk::SamplerCreateFlags(),
						vk::Filter::eLinear,
						vk::Filter::eLinear,
						vk::SamplerMipmapMode::eNearest,
						vk::SamplerAddressMode::eClampToEdge,
						vk::SamplerAddressMode::eClampToEdge,
						vk::SamplerAddressMode::eClampToEdge,
						0.0f,
						VK_TRUE, //VK_TRUE,	// nebo false
						device.getPhysDevice().getProperties().limits.maxSamplerAnisotropy, // 1.0f
						VK_FALSE,
						vk::CompareOp::eAlways,
						0.0f,
						1.0f,
						vk::BorderColor::eFloatOpaqueBlack,
						VK_FALSE
					)
				);
		}

		// creating buffer + AllocateAndBindMemoryObjectToBuffer
		createBuffer(layerSize, vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, hostBuff, hostBuffMemory);

		try {
			// MapUpdateAndUnmapHostVisibleMemory
			void* data = device.getDevice().mapMemory(hostBuffMemory, 0, layerSize, vk::MemoryMapFlags());
			memcpy(data, tex, static_cast<size_t>(layerSize));
			device.getDevice().unmapMemory(hostBuffMemory);
			stbi_image_free(tex);

			// BeginCommandBufferRecordingOperation + SetImageMemoryBarrier
			layoutConversion(texture.textureImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				{ vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite }, { vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer }, static_cast<uint32_t>(1), static_cast<uint32_t>(i));

			// CopyDataFromBufferToImage
			copyBuffToImage(hostBuff, texture.textureImage, vk::ImageLayout::eTransferDstOptimal, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), static_cast<uint32_t>(1), static_cast<uint32_t>(i));

		}
		catch (...) {
			stbi_image_free(tex);
			device.getDevice().destroyBuffer(hostBuff);
			device.getDevice().freeMemory(hostBuffMemory);
			throw runtime_error("Unspecified error in loadTextureCubemap.");
		}
		
		device.getDevice().destroyBuffer(hostBuff);
		device.getDevice().freeMemory(hostBuffMemory);

		layoutConversion(texture.textureImage, vk::Format::eR32G32B32A32Sfloat, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
			{ vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead }, { vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader }, static_cast<uint32_t>(1), static_cast<uint32_t>(i));

	}

	cout << "loadTextureCubemap(): Textures ";
	for (size_t i = 0; i < filename.size(); i++)
		cout  << filename[i] << " | ";
	cout << "are loaded.\n";
}


void HdaBuilder::layoutConversion(vk::Image img, vk::Format form, vk::ImageLayout oldLay, vk::ImageLayout newLay,
								  array<vk::AccessFlagBits, 2> accessMasks, array<vk::PipelineStageFlagBits, 2> stageMasks, uint32_t layerCount, uint32_t baseArr) {

	vk::CommandBuffer commandBuff =
		device.getDevice().allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				commandPools[actual_frame],
				vk::CommandBufferLevel::ePrimary,
				1
			)
		)[0];

	commandBuff.begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr  // pInheritanceInfo
		)
	);
	
	commandBuff.pipelineBarrier(
		stageMasks[0], stageMasks[1],
		vk::DependencyFlags(),
		0, {},
		0, {},
		1,
		&(const vk::ImageMemoryBarrier&)vk::ImageMemoryBarrier(
			accessMasks[0], accessMasks[1],
			oldLay, newLay,
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
			img,
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor,
				0, 1,	// baseMipLevel + levelCount 
				baseArr, layerCount //1	// baseArrayLayer + layerCount 
			)
		)
	);
	
	commandBuff.end();

	device.getGraphicsQueue().submit(
		vk::ArrayProxy<const vk::SubmitInfo>(
			1,
			&(const vk::SubmitInfo&)vk::SubmitInfo(
				0, nullptr,
				nullptr,
				1, &commandBuff,
				0, nullptr
			)
		)
	);

	device.getGraphicsQueue().waitIdle();
	device.getDevice().freeCommandBuffers(commandPools[actual_frame], commandBuff);
}


void HdaBuilder::copyBuffToImage(vk::Buffer srcBuff, vk::Image dstImg, vk::ImageLayout dstImgLay, uint32_t width, uint32_t height, uint32_t layerCount, uint32_t baseArr) {

	vk::CommandBuffer commandBuff =
		device.getDevice().allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				commandPools[actual_frame],
				vk::CommandBufferLevel::ePrimary,
				1
			)
		)[0];

	commandBuff.begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr  // pInheritanceInfo
		)
	);


	commandBuff.copyBufferToImage(
		srcBuff,	
		dstImg,
		dstImgLay,
		1,				// region count
		&(const vk::BufferImageCopy&)vk::BufferImageCopy(	// region
			0, 0, 0,			// buffer offset + buffer row length + buffer image height
			vk::ImageSubresourceLayers(
				vk::ImageAspectFlagBits::eColor,
				0, baseArr, layerCount // 1			// mip level + base array layer + layer count
			),
			vk::Offset3D(0, 0, 0),
			vk::Extent3D(width, height, 1)
		)
	);


	commandBuff.end();
	
	device.getGraphicsQueue().submit(
		vk::ArrayProxy<const vk::SubmitInfo>(
			1,
			&(const vk::SubmitInfo&)vk::SubmitInfo(
				0, nullptr,
				nullptr,
				1, &commandBuff,
				0, nullptr
			)
		)
	);

	device.getGraphicsQueue().waitIdle();
	device.getDevice().freeCommandBuffers(commandPools[actual_frame], commandBuff);
}


void HdaBuilder::loadCubemapSkybox(HdaModel::SceneObject* obj) {

	// TODO TODO lOADING
	vector<const char*> skyboxImagesSky3{
		"..\\models\\textures\\sky-8k\\px.hdr",		//skybox-right 
		"..\\models\\textures\\sky-8k\\nx.hdr",		//skybox-left" 
		"..\\models\\textures\\sky-8k\\py.hdr",		//skybox-top" 
		"..\\models\\textures\\sky-8k\\ny.hdr",		//skybox-bot" 
		"..\\models\\textures\\sky-8k\\pz.hdr",		//skybox-front"
		"..\\models\\textures\\sky-8k\\nz.hdr" };		//= sky-8k\\nz.hdr  skybox-back" };
	vector<const char*> skyboxImagesSky4{
		"..\\models\\textures\\skybox\\px.hdr",		//skybox-right 
		"..\\models\\textures\\skybox\\nx.hdr",		//skybox-left" 
		"..\\models\\textures\\skybox\\py.hdr",		//skybox-top" 
		"..\\models\\textures\\skybox\\ny.hdr",		//skybox-bot" 
		"..\\models\\textures\\skybox\\pz.hdr",		//skybox-front"
		"..\\models\\textures\\skybox\\nz.hdr" };		//= sky-8k\\nz.hdr  skybox-back" };

	loadMesh(obj->objectMesh, "..\\models\\sky-cube-def.obj", "..\\models");
	obj->objectTexture.resize(1);
	loadTextureCubemap(skyboxImagesSky4, obj->objectTexture[0]);
	obj->modelMatrix = glm::translate(glm::mat4{ 1.0f }, { 0, 0, 0 });
	obj->modelMatrix = glm::scale(obj->modelMatrix, { 300, 300, 300 });		// TODO TODO
	obj->objectDescriptSetLay = pipeline.createDescriptorSetLayout(0, 1);
	obj->objectPipelineLayout = pipeline.createPipelineLayout(vk::PipelineLayoutCreateFlags(), UINT32_MAX, &obj->objectDescriptSetLay, UINT32_MAX, nullptr);
	obj->objectPipeline = pipeline.createPipeline(vk::PipelineCreateFlags(), UINT32_MAX,
		array{  // pStages
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),
						vk::ShaderStageFlagBits::eVertex,  // stage
						pipeline.getSkyboxVertexShaderModule(),  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					},
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),
						vk::ShaderStageFlagBits::eFragment,  // stage
						pipeline.getSkyboxFragmentShaderModule(),  // module
						"main",  // pName
						nullptr// pSpecializationInfo
					},
		}.data(), nullptr, nullptr, nullptr, nullptr,
		&(const vk::PipelineRasterizationStateCreateInfo&)vk::PipelineRasterizationStateCreateInfo{  // pRasterizationState
					vk::PipelineRasterizationStateCreateFlags(),
					VK_FALSE,  // depthClampEnable
					VK_FALSE,  // rasterizerDiscardEnable
					vk::PolygonMode::eFill,  // polygonMode
					vk::CullModeFlagBits::eFront,  // cullMode PUVODNE eNone
					vk::FrontFace::eCounterClockwise,  // frontFace
					VK_FALSE,  // depthBiasEnable
					0.f,  // depthBiasConstantFactor
					0.f,  // depthBiasClamp
					0.f,  // depthBiasSlopeFactor
					1.f   // lineWidth
		}, nullptr, nullptr, nullptr, nullptr, obj->objectPipelineLayout, device.getRenderpass(), UINT32_MAX, nullptr, UINT32_MAX);
	obj->objectDescriptSets =
		createDescriptorSets(
			obj->objectDescriptSetLay,
			array{
				vk::DescriptorImageInfo(
					obj->objectTexture[0].textureSampler,
					obj->objectTexture[0].textureImageView,
					vk::ImageLayout::eShaderReadOnlyOptimal
				)
			}.data(),
			0,
			1,
			1
		);
}


void HdaBuilder::loadScene() {

	size_t s = sceneObjects.size();
	size_t idx = sceneObjects.size() - (s--);

	// TODO TODO 
	//sceneObjs[idx].createSceneObject(loadMesh(sceneObjs[idx].objMesh, "..\\models\\monkey_smooth.obj"), );
	

	//idx = sceneObjects.size() - (s--);
	loadMesh(sceneObjects[idx].objectMesh, "..\\models\\m-1.obj", "..\\models"); //"..\\models\\sponza\\sponza.obj", "..\\models\\sponza"
	sceneObjects[idx].objectTexture.resize(sceneObjects[idx].objectMesh.numMat);
	
	// TODO TODO make separated function
	for (uint32_t i = 0; i < sceneObjects[idx].objectMesh.numMat; i++) {
		cout << "texName[" << i << "]:" << sceneObjects[idx].objectMesh.texNames[i] << endl;
		if (sceneObjects[idx].objectMesh.texNames[i].empty())
			loadTexture(sceneObjects[idx].objectTexture[i], "..\\models\\textures\\default.png");
		else
			loadTexture(sceneObjects[idx].objectTexture[i], ("..\\models\\textures\\m-1tex\\" + sceneObjects[idx].objectMesh.texNames[i].replace(8, 1, "\\")).c_str());
		// TODO TODO smazat
		//loadTexture(sceneObjects[idx].objectTexture[i], ("..\\models\\sponza\\" + sceneObjects[idx].objectMesh.texNames[i].replace(8, 1, "\\").replace(sceneObjects[idx].objectMesh.texNames[i].size() - 3, 3, "png")).c_str());
	}
	sceneObjects[idx].multiTextureFlag = 1;
	sceneObjects[idx].nonRotateFlag = 1;
	sceneObjects[idx].texObjIndex = 1;
	sceneObjects[idx].modelMatrix = glm::rotate(glm::mat4{ 1.0f }, glm::radians(0.0f), glm::vec3(1, 0, 0));
	sceneObjects[idx].modelMatrix = glm::translate(sceneObjects[idx].modelMatrix, { 0, -10, 0 });
	sceneObjects[idx].modelMatrix = glm::scale(sceneObjects[idx].modelMatrix, glm::vec3(0.05f));
	sceneObjects[idx].objectDescriptSetLay = pipeline.createDescriptorSetLayout(0, 1);
	sceneObjects[idx].objectPipelineLayout = pipeline.createPipelineLayout(vk::PipelineLayoutCreateFlags(), UINT32_MAX, &sceneObjects[idx].objectDescriptSetLay, UINT32_MAX, nullptr);
	sceneObjects[idx].objectPipeline = pipeline.createPipeline(vk::PipelineCreateFlags(), UINT32_MAX, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
															   sceneObjects[idx].objectPipelineLayout, device.getRenderpass(), UINT32_MAX, nullptr, UINT32_MAX);

	// Create one descriptor for each model texture. The number of textures is predefined in the .mtl file, so it is possible to create a vector of descriptors with a given size. 
	// The loop loops through all the subfaces and creates a descriptor for all those with the same texture, which is stored at position currentTexIdx in the descriptor vector, 
	// which is the texture index. The same currentTexIdx index is used for the objectTexture vector created above.
	sceneObjects[idx].dsv.resize(sceneObjects[idx].objectMesh.numMat);
	auto currentTexIdx = UINT32_MAX;
	for (int32_t i = 0; i < sceneObjects[idx].objectMesh.info.size(); i++) {
		
		auto thisTexIdx = sceneObjects[idx].objectMesh.info[i].textureIndex;
		if (thisTexIdx != currentTexIdx && thisTexIdx <= sceneObjects[idx].objectMesh.numMat) {
			currentTexIdx = thisTexIdx;

			sceneObjects[idx].dsv[currentTexIdx] = createDescriptorSets(
				sceneObjects[idx].objectDescriptSetLay,
				array{
					vk::DescriptorImageInfo(
						sceneObjects[idx].objectTexture[currentTexIdx].textureSampler,
						sceneObjects[idx].objectTexture[currentTexIdx].textureImageView,
						vk::ImageLayout::eShaderReadOnlyOptimal
					)
				}.data(),
				0,
				1,
				1
			);
		}
	}
	

	idx = sceneObjects.size() - (s--);
	loadMesh(sceneObjects[idx].objectMesh, "..\\models\\m-2.obj", "..\\models");
	sceneObjects[idx].objectTexture.resize(1);
	loadTexture(sceneObjects[idx].objectTexture[0], "..\\models\\textures\\default.png");
	sceneObjects[idx].nonTextureFlag = 1;

	sceneObjects[idx].modelMatrix = glm::rotate(glm::mat4{ 1.0f }, glm::radians(0.0f), glm::vec3(1, 0, 0));
	sceneObjects[idx].modelMatrix = glm::translate(sceneObjects[idx].modelMatrix, { -35.0f, 25.0f, 0.0f });
	sceneObjects[idx].objectDescriptSetLay = pipeline.createDescriptorSetLayout(0, 1);
	sceneObjects[idx].objectPipelineLayout = pipeline.createPipelineLayout(vk::PipelineLayoutCreateFlags(), UINT32_MAX, &sceneObjects[idx].objectDescriptSetLay, UINT32_MAX, nullptr);
	sceneObjects[idx].objectPipeline = pipeline.createPipeline(vk::PipelineCreateFlags(), UINT32_MAX, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
		sceneObjects[idx].objectPipelineLayout, device.getRenderpass(), UINT32_MAX, nullptr, UINT32_MAX);
	sceneObjects[idx].objectDescriptSets =
		createDescriptorSets(
			sceneObjects[idx].objectDescriptSetLay,
			array{
				vk::DescriptorImageInfo(
					sceneObjects[idx].objectTexture[0].textureSampler,
					sceneObjects[idx].objectTexture[0].textureImageView,
					vk::ImageLayout::eShaderReadOnlyOptimal
				)
			}.data(), 0, 1, 1
		);

	// skybox is always loaded and rendered last
	idx = sceneObjects.size() - (s--);
	loadCubemapSkybox(&sceneObjects[idx]);
	
	// reset variable s
	s = sceneObjects.size();
}


void HdaBuilder::calculateLightColor(glm::vec4 lightC, glm::vec4 diffuse, glm::vec4 ambient) {

	lightColor = lightC;
	diffuseColor = lightColor * diffuse;
	ambientColor = diffuseColor * ambient;
}


void HdaBuilder::calculateAdditionalData() {

	requiredAlignmentScene = static_cast<uint32_t>(calcRequiredAligment(sizeof(HdaModel::SceneUniformData)));
	requiredAlignmentMaterial = static_cast<uint32_t>(calcRequiredAligment(sizeof(HdaModel::MaterialLightUniformData)));
	sceneUniformSize = sizeof(HdaModel::SceneUniformData);
	materialUniformSize = sizeof(HdaModel::MaterialLightUniformData);

	// light colors
	calculateLightColor(glm::vec4(10.0f, 9.985f, 9.990f, 1.0f), glm::vec4(2.5f, 2.5f, 2.5f, 1.0f), glm::vec4(0.12f, 0.12f, 0.12f, 1.0f));
	// glm::vec4(1.0f, 0.985f, 0.990f, 1.0f) puvodni		glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)
	sceneObjectsSize = static_cast<uint32_t>(sceneObjects.size());
}


void HdaBuilder::setUniformStructures(HdaModel::SceneObject* o, vk::CommandBuffer* cmdBuffs, float t, uint32_t dynamicUniformOffset, uint32_t dynamicMaterialLightUniformOffset, HdaModel::MaterialLightUniformData* matlightData, HdaModel::SceneUniformData* sceneD) {

	HdaModel::PushConstants constants{};
	array<string, 5> methodNames;
	methodNames[0] = "\nALGORITHM: Reinhard\n";
	methodNames[1] = "\nALGORITHM: Hejl-Dawson\n";
	methodNames[2] = "\nALGORITHM: Hable - Uncharted 2\n";
	methodNames[3] = "\nALGORITHM: ACES TMO\n";
	methodNames[4] = "\nALGORITHM: Modified Reinhard\n";

	// TODO TODO hodit do vlastni funkce
	if (window.getKeyPressedXFlag() == true) {

		hdrOnFlag == 0 ? hdrOnFlag = 1 : hdrOnFlag = 0;
		hdrOnFlag == 1 ? cout << "\nHDR: ON" << endl : cout << "\nHDR: OFF" << endl;
		cout << methodNames[chooseMethodFlag];
		window.setKeyPressedXFlag();
	}
	sceneD->hdrOnFlag = hdrOnFlag;

	if (window.getKeyPressedMFlag() == true) {

		if (chooseMethodFlag == 4)
			chooseMethodFlag = 0;
		else
			chooseMethodFlag += 1;
		window.setKeyPressedMFlag();

		cout << methodNames[chooseMethodFlag];
	}
	sceneD->chooseMethodFlag = chooseMethodFlag;

	sceneD->nontextureFlag = 0;

	// STATIC uniform buffer with model view projection data
	glm::vec3 cameraPos{ 0.0f, 0.0f, 0.0f };
	glm::mat4 skyboxView;
	HdaModel::ProjectionUniformData modelviewProjection = HdaModel::projectionCalculation(o->modelMatrix, window.getGLFWwindow(), &skyboxView, cameraPos, sceneD, window.getKeyPressedXFlag(), &exposure);
	memcpy(uniformBuffsMemoryPointer[actual_frame], &modelviewProjection, sizeof(modelviewProjection));
	sceneD->exposure = exposure;

	// DYNAMIC uniform structure bind to dynamic uniform buffer with lighting and material data
	HdaModel::loadLightData(matlightData, ambientColor, diffuseColor, glm::vec4(1.0f), array{ glm::vec4{ -15.0f, 25.0f, 0.0f, 1.0f } , glm::vec4{ -25.0f, 25.0f, 0.0f, 1.0f } }, array{ glm::vec4{ 15.0f, 0.0f, 0.0f, 0.0f } , glm::vec4{ -15.0f, 0.0f, 0.0f, 0.0f } });

	// selection of objects that should not rotate
	if (o->nonRotateFlag == 0)
		constants.modelMatrix = glm::rotate(o->modelMatrix, t * glm::radians(30.0f), glm::vec3(0, 1, 0)); //sceneObjects[i].modelMatrix;
	else
		constants.modelMatrix = o->modelMatrix;	//constants.modelMatrix = glm::translate(constants.modelMatrix, { 2 + (5 * cos(5)), 2 + (5 * sin(5)), 0 });

	// selection of objects that should not be textured
	if (o->nonTextureFlag == 1) {
		sceneD->nontextureFlag = 1;
	}

	constants.cameraPosition = cameraPos;

	// prepare matricies for calculation of normal matrix in vertex shader
	sceneD->modelView = modelviewProjection.view * constants.modelMatrix;
	sceneD->normalMatrix = glm::transpose(glm::inverse(sceneD->modelView));
	sceneD->normalMatrixWorld = glm::transpose(glm::inverse(constants.modelMatrix));

	cmdBuffs->pushConstants(o->objectPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(HdaModel::PushConstants), &constants);

	mapMemoryToUniformBuffer(sceneUniformBuffMemory, sceneUniformSize, 2, *matlightData, 0, *sceneD, SELECT_SCENEDATA);
	mapMemoryToUniformBuffer(materialLightUniformBuffMemory, materialUniformSize, 2, *matlightData, 0, *sceneD, SELECT_MATLIGHTDATA);

	// BIND descriptors with dynamic uniform buffer
	uint32_t offsets[] = { dynamicUniformOffset, dynamicMaterialLightUniformOffset };
	cmdBuffs->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, o->objectPipelineLayout, 0, 1, &o->dsv[0][actual_frame], 2, offsets);
}


void HdaBuilder::mapMemoryToUniformBuffer(vk::DeviceMemory devMem, size_t sizeofStructure, uint32_t numOfSizes, HdaModel::MaterialLightUniformData matlightData, uint32_t offsetN, HdaModel::SceneUniformData sceneData, uint32_t structSelect) {

	char* data{};
	if (device.getDevice().mapMemory(devMem, 0, calcRequiredAligment(sizeofStructure) * numOfSizes, vk::MemoryMapFlags(), (void**)&data) != vk::Result::eSuccess)
		throw runtime_error("Function vk::Device::mapMemory() crash.\n");
	data += calcRequiredAligment(sizeofStructure) * offsetN;
	if (structSelect == 1)
		memcpy(data, &sceneData, sizeofStructure);
	else
		memcpy(data, &matlightData, sizeofStructure);
	device.getDevice().unmapMemory(devMem);
}


void HdaBuilder::drawMultiTexturedObjects(HdaModel::SceneObject* o, vk::CommandBuffer* cmdBuffs, float t) {

	uint32_t currentTexIdx = UINT32_MAX;

	uint32_t dynamicUniformOffset = 0;  // requiredAlignmentScene * 0;
	uint32_t dynamicMaterialLightUniformOffset = 0; // requiredAlignmentMaterial * 0;
	
	HdaModel::MaterialLightUniformData matlightData{};
	HdaModel::SceneUniformData sceneData{};

	uint32_t infoSize = static_cast<uint32_t>(o->objectMesh.info.size());

	// TODO TODO
	setUniformStructures(o, cmdBuffs, t, dynamicUniformOffset, dynamicMaterialLightUniformOffset, &matlightData, &sceneData);

	// loop through grouoped faces (triangles)
	for (uint32_t i = 0, k = 0; i < infoSize; i++) {

		auto thisTexIdx = o->objectMesh.info[i].textureIndex;
		if (thisTexIdx != currentTexIdx && thisTexIdx <= o->objectMesh.numMat) {

			currentTexIdx = thisTexIdx;

			// set material properties
			HdaModel::loadMaterialData(&matlightData, o->objectMesh.mats[currentTexIdx].ambient, o->objectMesh.mats[currentTexIdx].diffuse, o->objectMesh.mats[currentTexIdx].specular, o->objectMesh.mats[currentTexIdx].shi);

			dynamicMaterialLightUniformOffset = static_cast<uint32_t>(requiredAlignmentMaterial * k);

			mapMemoryToUniformBuffer(materialLightUniformBuffMemory, materialUniformSize, o->objectMesh.submeshCnt, matlightData, k, sceneData, SELECT_MATLIGHTDATA);		
			
			uint32_t offsets[] = { dynamicUniformOffset, dynamicMaterialLightUniformOffset };
			cmdBuffs->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, o->objectPipelineLayout, 0, 1, &o->dsv[currentTexIdx][actual_frame], 2, offsets);
			
			k++;
		}
	
		cmdBuffs->draw(
				static_cast<uint32_t>(o->objectMesh.info[i].vertexCnt),  // vertexCount
				1,  // instanceCount
				o->objectMesh.info[i].firstVertex,  // firstVertex
				0   // firstInstance
			);

		/*cmdBuffs->drawIndexed(static_cast<uint32_t>(o->objectMesh.info[i].vertexCnt),1,o->objectMesh.info[i].firstVertex,0,0);*/
	}
}


// TODO TODO
void HdaBuilder::drawSingleTexturedObjects(HdaModel::SceneObject* o, vk::CommandBuffer* cmdBuffs, float t, int i) {

	// DYNAMIC uniform structure bind to dynamic uniform buffer with additional scene data
	HdaModel::SceneUniformData sceneData;
	array<string, 5> methodNames;
	methodNames[0] = "\nALGORITHM: Reinhard TMO\n";
	methodNames[1] = "\nALGORITHM: Hejl-Dawson TMO\n";
	methodNames[2] = "\nALGORITHM: Hable - Uncharted 2 TMO\n";
	methodNames[3] = "\nALGORITHM: ACES TMO\n";
	methodNames[4] = "\nALGORITHM: Modified Reinhard TMO\n";

	// TODO TODO hodi do vlastní funkce
	if (window.getKeyPressedXFlag() == true) {

		hdrOnFlag == 0 ? hdrOnFlag = 1 : hdrOnFlag = 0;
		hdrOnFlag == 1 ? cout << "\nHDR: ON" << endl : cout << "\nHDR: OFF" << endl;
		cout << methodNames[chooseMethodFlag];
		window.setKeyPressedXFlag();
	}
	sceneData.hdrOnFlag = hdrOnFlag;

	if (window.getKeyPressedMFlag() == true) {

		if (chooseMethodFlag == 4)
			chooseMethodFlag = 0;
		else
			chooseMethodFlag += 1;

		window.setKeyPressedMFlag();
		
		cout << methodNames[chooseMethodFlag];
	}
	sceneData.chooseMethodFlag = chooseMethodFlag;
	sceneData.nontextureFlag = 0;
	
	// STATIC uniform buffer with model view projection data
	glm::vec3 cameraPos{ 0.0f, 0.0f, 0.0f };
	glm::mat4 skyboxView;
	HdaModel::ProjectionUniformData modelviewProjection = HdaModel::projectionCalculation(o->modelMatrix, window.getGLFWwindow(), &skyboxView, cameraPos, &sceneData, window.getKeyPressedXFlag(), &exposure);
	memcpy(uniformBuffsMemoryPointer[actual_frame], &modelviewProjection, sizeof(modelviewProjection));
	sceneData.exposure = exposure;

	// DYNAMIC uniform structure bind to dynamic uniform buffer with lighting and material data
	HdaModel::MaterialLightUniformData matlightData;
	HdaModel::loadLightData(&matlightData, ambientColor, diffuseColor, glm::vec4(1.0f), array{ glm::vec4{ -15.0f, 25.0f, 0.0f, 1.0f } , glm::vec4{ -25.0f, 25.0f, 0.0f, 1.0f } }, array{ glm::vec4{ 15.0f, 0.0f, 0.0f, 0.0f } , glm::vec4{ -15.0f, 0.0f, 0.0f, 0.0f } });
	HdaModel::loadMaterialData(&matlightData, glm::vec4(0.6f, 0.6f, 0.6f, 0.0f), glm::vec4(0.6f, 0.6f, 0.6f, 0.0f), glm::vec4(0.5f, 0.5f, 0.5f, 0.0f), 16.0f);


	HdaModel::PushConstants constants{};

	// the skybox requires its own view matrix, which lacks a translational component
	// because the skybox must always be in the middle of the camera and we are not allowed to approach it
	// 
	// depending on the type of the object, we choose different structures for push constants
	if (i == (sceneObjectsSize - 1)) {	// skybox is rendered as last object because depth testing 

		HdaModel::SkyboxPushConstants skyboxConstants{};
		skyboxConstants.modelMatrix = o->modelMatrix;
		skyboxConstants.viewMatrix = skyboxView;

		cmdBuffs->pushConstants(o->objectPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(HdaModel::SkyboxPushConstants), &skyboxConstants);
	}
	else {

		// selection of objects that should not rotate
		if (o->nonRotateFlag == 0)
			constants.modelMatrix = glm::rotate(o->modelMatrix, t * glm::radians(30.0f), glm::vec3(0, 1, 0)); //o->modelMatrix;
		else
			constants.modelMatrix = o->modelMatrix;	//constants.modelMatrix = glm::translate(constants.modelMatrix, { 2 + (5 * cos(5)), 2 + (5 * sin(5)), 0 });

		// selection of objects that should not be textured
		if (o->nonTextureFlag == 1) {
			sceneData.nontextureFlag = 1;
		}

		constants.cameraPosition = cameraPos;

		// prepare matricies for calculation of normal matrix in vertex shader
		sceneData.modelView = modelviewProjection.view * constants.modelMatrix;
		sceneData.normalMatrix = glm::transpose(glm::inverse(sceneData.modelView));
		sceneData.normalMatrixWorld = glm::transpose(glm::inverse(constants.modelMatrix));

		cmdBuffs->pushConstants(o->objectPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(HdaModel::PushConstants), &constants);
	}

	mapMemoryToUniformBuffer(sceneUniformBuffMemory, sceneUniformSize, sceneObjectsSize, matlightData, i, sceneData, SELECT_SCENEDATA);
	mapMemoryToUniformBuffer(materialLightUniformBuffMemory, materialUniformSize, sceneObjectsSize, matlightData, i, sceneData, SELECT_MATLIGHTDATA);
	uint32_t dynamicUniformOffset = requiredAlignmentScene * i;
	uint32_t dynamicMaterialLightUniformOffset = requiredAlignmentMaterial * i;

	// BIND descriptors with dynamic uniform buffer
	uint32_t offsets[] = { dynamicUniformOffset, dynamicMaterialLightUniformOffset };
	cmdBuffs->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, o->objectPipelineLayout, 0, 1, &o->objectDescriptSets[actual_frame], 2, offsets);

	cmdBuffs->draw(
		static_cast<uint32_t>(o->objectMesh.meshVertices.size()),  // vertexCount
		1,  // instanceCount
		0,  // firstVertex
		0   // firstInstance
	);

	// TODO TODO smazat
	/*cmdBuffs->drawIndexed(static_cast<uint32_t>(sceneObjects[i].objectMesh.meshIndices.size()),1,0,0,0);*/
}

void HdaBuilder::drawScene(vk::CommandBuffer* cmdBuffs) {


	vk::Pipeline currentPipe{};
	vk::Buffer currentVertexBuff{};

	for (uint32_t i = 0; i < sceneObjectsSize; i++) {

		// time counter
		static auto startT = std::chrono::high_resolution_clock::now();
		auto currentT = std::chrono::high_resolution_clock::now();
		float t = std::chrono::duration<float, std::chrono::seconds::period>(currentT - startT).count();

		// check if last pipe and vertex buffer is not same for higher performance
		if (sceneObjects[i].objectPipeline != currentPipe) {

			cmdBuffs->bindPipeline(vk::PipelineBindPoint::eGraphics, sceneObjects[i].objectPipeline);

			currentPipe = sceneObjects[i].objectPipeline;
		}

		if (sceneObjects[i].objectMesh.vertexBuff != currentVertexBuff) {

			vk::Buffer vertexBuffers[] = { sceneObjects[i].objectMesh.vertexBuff };
			vk::DeviceSize offsets[] = { 0 };
			cmdBuffs->bindVertexBuffers(0, 1, vertexBuffers, offsets);

			// if draw indexed
			//cmdBuffs->bindIndexBuffer(sceneObjects[i].objectMesh.indexBuff, 0, vk::IndexType::eUint32);

			currentVertexBuff = sceneObjects[i].objectMesh.vertexBuff;
		}

		if (sceneObjects[i].multiTextureFlag == 1)
			drawMultiTexturedObjects(&sceneObjects[i], cmdBuffs, t);
		else
			drawSingleTexturedObjects(&sceneObjects[i], cmdBuffs, t, i);
	}
}


void HdaBuilder::render() {

	vk::Result result;

	// wait for rendering fence
	result =
		device.getDevice().waitForFences(
			renderCompleteFences[actual_frame],
			VK_TRUE,  // waitAll
			uint64_t(4e9)  // timeout
		);
	if (result != vk::Result::eSuccess) {
		if (result == vk::Result::eTimeout)
			throw runtime_error("Task on GPU timeout.");
		throw runtime_error("waitForFences() failed with error " + to_string(result) + ".");
	}
	if (window.getFramebufferResizedFlag()) {
		window.setFramebufferResizedFlag();
		recreateSwapchain();
	}
	device.getDevice().resetFences(renderCompleteFences[actual_frame]);

	// get next image index for render and presentation
	uint32_t imageIndex;
	result = device.getDevice().acquireNextImageKHR(
		swapchain.getSwapchain(), uint64_t(4e9),
		presentCompleteSemaphores[actual_frame], nullptr, &imageIndex
	);
	if (result == vk::Result::eTimeout) {
		throw runtime_error("Vulkan error: vk::Device::acquireNextImageKHR() timed out.");
	}
	else if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorIncompatibleDisplayKHR) { // || result == vk::Result::eErrorIncompatibleDisplayKHR
		cout << "Recreate window" << std::endl;
		recreateSwapchain(); // this will recreate size of window, surface and triangle
		return;
	}

	commandBuffers[actual_frame].reset();

	// // // //
	// recordording command buffer begin
	commandBuffers[actual_frame].begin(
		vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
			nullptr  // pInheritanceInfo
		)
	);

	commandBuffers[actual_frame].beginRenderPass(
		vk::RenderPassBeginInfo(
			device.getRenderpass(),
			swapchain.getFramebuffers()[imageIndex],  // framebuffer with right image index
			vk::Rect2D(vk::Offset2D(0, 0), swapchain.getSurfaceExtent()),  // renderArea
			2,  // clearValueCount
			array{  // pClearValues
				vk::ClearValue(array<float,4>{0.447f, 0.451f, 0.565f, 1.f}),	// {0.678f, 0.973f, 0.992f, 1.f})
				vk::ClearValue({1.0f, 0}),	// The initial value at each point in the depth buffer should be the furthest possible depth, which is 1.0.
			}.data()
			),
		vk::SubpassContents::eInline
	);


	drawScene(&commandBuffers[actual_frame]);
	

	/*	TODO TODO delete
	// recording rendering commands
	commandBuffers[actual_frame].bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getPipeline());  // bind pipeline

	vk::Buffer vertexBuffers[] = { rectangleMesh.vertexBuff};
	vk::DeviceSize offsets[] = { 0 };
	commandBuffers[actual_frame].bindVertexBuffers(0, 1, vertexBuffers, offsets);

	//commandBuffers[actual_frame].bindIndexBuffer(rectangleMesh.indexBuff, 0, vk::IndexType::eUint32);

	// TODO TODO
	glm::mat4 mod = glm::rotate(glm::mat4{ 1.0f }, glm::radians(0.0f), glm::vec3(0, 1, 0));
	mod = glm::translate(mod, { 0, -50, 0 });
	HdaModel::ProjectionUniformData modelviewProjection = HdaModel::projectionCalculation(mod, window.getGLFWwindow(), counter);
	memcpy(uniformBuffsMemoryPointer[actual_frame], &modelviewProjection, sizeof(modelviewProjection));

	//commandBuffers[actual_frame].drawIndexed(
		//static_cast<uint32_t>(rectangleMesh.meshIndices.size()), // num of indices   PREDTIM: indices.size()
		//1,	// num of instances
		//0,	// offset into index buffer (first index)
		//0,	// offset which add to indices in index buffer
		//0	// offset fo instancing
	//);

	commandBuffers[actual_frame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.getPipelineLayout(), 0, 1, &descriptorSets[actual_frame], 0, nullptr);
	//commandBuffers[actual_frame].pushConstants(pipeline.getPipelineLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &constants);

	commandBuffers[actual_frame].draw(  // draw single triangle
		static_cast<uint32_t>(rectangleMesh.meshVertices.size()),  // vertexCount predtim 3
		1,  // instanceCount
		0,  // firstVertex
		0   // firstInstance
	);
	*/

	commandBuffers[actual_frame].endRenderPass();
	commandBuffers[actual_frame].end();
	// recordording command buffer end
	// // // //


	// submit frame for render
	device.getGraphicsQueue().submit(
		vk::ArrayProxy<const vk::SubmitInfo>(
			1,
			&(const vk::SubmitInfo&)vk::SubmitInfo(
				1, &presentCompleteSemaphores[actual_frame],  // waitSemaphoreCount + pWaitSemaphores +
				&(const vk::PipelineStageFlags&)vk::PipelineStageFlags(  // pWaitDstStageMask
					vk::PipelineStageFlagBits::eColorAttachmentOutput),
				1, &commandBuffers[actual_frame],  // commandBufferCount + pCommandBuffers
				1, &renderCompleteSemaphores[actual_frame]  // signalSemaphoreCount + pSignalSemaphores
			)
			),
		renderCompleteFences[actual_frame]
	);

	// present
	result =
		device.getPresentationQueue().presentKHR(
			&(const vk::PresentInfoKHR&)vk::PresentInfoKHR(
				1, &renderCompleteSemaphores[actual_frame],  // waitSemaphoreCount + pWaitSemaphores
				1, &swapchain.getSwapchain(), &imageIndex,  // swapchainCount + pSwapchains + pImageIndices
				nullptr  // pResults
			)
		);

	if (result != vk::Result::eSuccess) {
		if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || window.getFramebufferResizedFlag()) {
			cout << "present result: Suboptimal\nrecreate\n" << endl;
			window.setFramebufferResizedFlag();
			recreateSwapchain();
		}
		else
			throw runtime_error("Vulkan error: vkQueuePresentKHR() failed with error ");
	}

	fps();

	actual_frame = (actual_frame + 1) % PARALLEL_FRAMES;
}

void HdaBuilder::fps() {

	// TODO TODO delete
	/*currentT = glfwGetTime();
	diff = currentT - lastT;

	if (diff >= 2) {
		frameRate = frames / diff;
		cout << "FPS: " << frameRate << endl;
		lastT = currentT;
		frames = 0;
	}*/
	if (frames == 0)
		lT = chrono::high_resolution_clock::now();
	else {
		cT = chrono::high_resolution_clock::now();
		auto d = cT - lT;
		if (d >= chrono::seconds(6)) {
			auto fr = frames / chrono::duration<double>(d).count();
			cout << "\r" << "FPS: " << fr;
			cout << " | exposure: " << exposure;
			frames = 0.0;
			lT = cT;
		}
	}

	frames++;
}