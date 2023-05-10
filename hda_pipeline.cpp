#include "hda_pipeline.hpp"



const uint32_t vertexShaderSpirv[] = {
	#include "shader.vert.spv"
};
const uint32_t fragmentShaderSpirv[] = {
	#include "shader.frag.spv"
};

const uint32_t skyboxVertexShaderSpirv[] = {
	#include "skybox.vert.spv"
};
const uint32_t skyboxFragmentShaderSpirv[] = {
	#include "skybox.frag.spv"
};



/*
*
* Most of the initialization features are largely inspired by freely available tutorials
* for understanding the basic principles of working with Vulkan objects:
*
* https://vkguide.dev
* https://vulkan-tutorial.com/Introduction
* https://www.root.cz/serialy/tutorial-vulkan/
* https://github.com/blurrypiano/littleVulkanEngine/tree/tut27
* https://github.com/amengede/getIntoGameDev/tree/main/vulkan
* https://kohiengine.com
*/

HdaPipeline::HdaPipeline(HdaInstanceGpu& dev, HdaSwapchain& swa) : device{ dev }, swapchain{ swa } {

	cout << "HdaPipeline(): constructor\n";
}

HdaPipeline::~HdaPipeline() {

	cout << "HdaPipeline: Destructor\n";

	if (eCh == 1)
		cleanupPipeline();
}

void HdaPipeline::initPipeline() {

	eCh = 1;

	createShaderModules();
}

void HdaPipeline::cleanupPipeline() {

	device.getDevice().destroyDescriptorSetLayout(descriptorSetLayout);
	device.getDevice().destroyPipeline(pipeline);
	device.getDevice().destroyPipelineLayout(pipelineLayout);
	device.getDevice().destroyShaderModule(fragmentShaderModule);
	device.getDevice().destroyShaderModule(vertexShaderModule);
	device.getDevice().destroyShaderModule(skyboxFragmentShaderModule);
	device.getDevice().destroyShaderModule(skyboxVertexShaderModule);
	device.getDevice().destroyShaderModule(hdrFragmentShaderModule);
	device.getDevice().destroyShaderModule(hdrVertexShaderModule);
}


vk::DescriptorSetLayout HdaPipeline::createDescriptorSetLayout(int descriptType, uint32_t descrBind) {

	vk::DescriptorSetLayout descriptorSetLay;

	descriptorSetLay =
		device.getDevice().createDescriptorSetLayout(
			vk::DescriptorSetLayoutCreateInfo(
				vk::DescriptorSetLayoutCreateFlags(),		
				descriptType == 0 ? 4 : 1,//binding count	// CHANGED
				descriptType == 0 ? array{
					vk::DescriptorSetLayoutBinding{
						0,
						vk::DescriptorType::eUniformBuffer,
						1,
						vk::ShaderStageFlagBits::eVertex,
						nullptr
					},
					vk::DescriptorSetLayoutBinding{		// CHANGED
						1,
						vk::DescriptorType::eUniformBufferDynamic,
						1,
						vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
						nullptr
					},
					vk::DescriptorSetLayoutBinding{		// CHANGED
						2,
						vk::DescriptorType::eUniformBufferDynamic,
						1,
						vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
						nullptr
					},
					vk::DescriptorSetLayoutBinding{
						3,
						vk::DescriptorType::eCombinedImageSampler,
						descrBind,	//1
						vk::ShaderStageFlagBits::eFragment,
						nullptr
					}
				}.data() : array{
					vk::DescriptorSetLayoutBinding{
						0,
						vk::DescriptorType::eUniformBuffer,
						1,
						vk::ShaderStageFlagBits::eVertex,
						nullptr
					}
				}.data()// bindings
			)
		);
	cout << "createDescriptorSetLayout(): Layout is created.\n";

	return descriptorSetLay;
}


vk::Pipeline HdaPipeline::createPipeline(vk::PipelineCreateFlags flags, uint32_t stgCnt, const vk::PipelineShaderStageCreateInfo* shaders, const vk::PipelineVertexInputStateCreateInfo* vrtxIn, 
										 const vk::PipelineInputAssemblyStateCreateInfo* assemIn, const vk::PipelineTessellationStateCreateInfo* tess, const vk::PipelineViewportStateCreateInfo* viewPort, 
										 const vk::PipelineRasterizationStateCreateInfo* raster, const vk::PipelineMultisampleStateCreateInfo* ms, const vk::PipelineDepthStencilStateCreateInfo* depth, 
										 const vk::PipelineColorBlendStateCreateInfo* blend, const vk::PipelineDynamicStateCreateInfo* dynamic, vk::PipelineLayout pipLay, vk::RenderPass rp, uint32_t subp,
										 vk::Pipeline baseHandle, uint32_t baseIdx) {

	vk::Pipeline pipe;

	pipe = 
		device.getDevice().createGraphicsPipeline(
			nullptr,  // pipelineCache
			vk::GraphicsPipelineCreateInfo(
				flags,

				// shader stages
				stgCnt == UINT32_MAX ? 2 : stgCnt,  // stageCount
				shaders ==  nullptr ?
				array{  // pStages
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),
						vk::ShaderStageFlagBits::eVertex,  // stage
						vertexShaderModule,  // module
						"main",  // pName
						nullptr  // pSpecializationInfo
					},
					vk::PipelineShaderStageCreateInfo{
						vk::PipelineShaderStageCreateFlags(),
						vk::ShaderStageFlagBits::eFragment,  // stage
						fragmentShaderModule,  // module
						"main",  // pName
						nullptr// pSpecializationInfo
					},
				}.data() : shaders,

				// vertex input
				vrtxIn == nullptr ?
				&(const vk::PipelineVertexInputStateCreateInfo&)vk::PipelineVertexInputStateCreateInfo{  // pVertexInputState
					vk::PipelineVertexInputStateCreateFlags(),
					1,        // vertexBindingDescriptionCount    // 1
					&(HdaModel::Vertex::getBindingDescription()), // pVertexBindingDescriptions		  // getBindingDescription()
					4,        // vertexAttributeDescriptionCount  // 3
					HdaModel::Vertex::getAttributeDescription().data() // pVertexAttributeDescriptions	  // getAttributeDescription()
				} : vrtxIn,

				// input assembly
				assemIn == nullptr ?
				&(const vk::PipelineInputAssemblyStateCreateInfo&)vk::PipelineInputAssemblyStateCreateInfo{  // pInputAssemblyState
					vk::PipelineInputAssemblyStateCreateFlags(),
					vk::PrimitiveTopology::eTriangleList,  // topology
					VK_FALSE  // primitiveRestartEnable
				} : assemIn,

				// tessellation
				tess == nullptr ? nullptr : tess, // pTessellationState

				// viewport
				viewPort == nullptr ?
				&(const vk::PipelineViewportStateCreateInfo&)vk::PipelineViewportStateCreateInfo{  // pViewportState
					vk::PipelineViewportStateCreateFlags(),
					1,  // viewportCount
					array{  // pViewports
						vk::Viewport(0.f, 0.f, float(swapchain.getSurfaceExtent().width), float(swapchain.getSurfaceExtent().height), 0.f, 1.f),
					}.data(),
					1,  // scissorCount
					array{  // pScissors
						vk::Rect2D(vk::Offset2D(0,0), swapchain.getSurfaceExtent())
					}.data(),
				} : viewPort,

				// rasterization
				raster == nullptr ?
				&(const vk::PipelineRasterizationStateCreateInfo&)vk::PipelineRasterizationStateCreateInfo{  // pRasterizationState
					vk::PipelineRasterizationStateCreateFlags(),
					VK_FALSE,  // depthClampEnable
					VK_FALSE,  // rasterizerDiscardEnable
					vk::PolygonMode::eFill,  // polygonMode
					vk::CullModeFlagBits::eNone,  // cullMode PUVODNE eNone
					vk::FrontFace::eCounterClockwise,  // frontFace
					VK_FALSE,  // depthBiasEnable
					0.f,  // depthBiasConstantFactor
					0.f,  // depthBiasClamp
					0.f,  // depthBiasSlopeFactor
					1.f   // lineWidth
				} : raster,

				// multisampling
				ms == nullptr ? 
				&(const vk::PipelineMultisampleStateCreateInfo&)vk::PipelineMultisampleStateCreateInfo{  // pMultisampleState
					vk::PipelineMultisampleStateCreateFlags(),
					vk::SampleCountFlagBits::e1,  // rasterizationSamples
					VK_FALSE,  // sampleShadingEnable
					0.f,       // minSampleShading
					nullptr,   // pSampleMask
					VK_FALSE,  // alphaToCoverageEnable
					VK_FALSE   // alphaToOneEnable
				} : ms,

				// depth and stencil
				depth == nullptr ?
				&(const vk::PipelineDepthStencilStateCreateInfo&)vk::PipelineDepthStencilStateCreateInfo{
					vk::PipelineDepthStencilStateCreateFlags(),
					VK_TRUE,
					VK_TRUE,
					vk::CompareOp::eLessOrEqual,	// before was eLess and cubemap texture blinking
					VK_FALSE,
					VK_FALSE,
					{},
					{},
					0.0f,
					1.0f
				} : depth,  // pDepthStencilState

				// blending
				blend == nullptr ?
				&(const vk::PipelineColorBlendStateCreateInfo&)vk::PipelineColorBlendStateCreateInfo{  // pColorBlendState
					vk::PipelineColorBlendStateCreateFlags(),
					VK_FALSE,  // logicOpEnable
					vk::LogicOp::eClear,  // logicOp
					1,  // attachmentCount
					array{  // pAttachments
						vk::PipelineColorBlendAttachmentState{
							VK_FALSE,  // blendEnable
							vk::BlendFactor::eZero,  // srcColorBlendFactor
							vk::BlendFactor::eZero,  // dstColorBlendFactor
							vk::BlendOp::eAdd,       // colorBlendOp
							vk::BlendFactor::eZero,  // srcAlphaBlendFactor
							vk::BlendFactor::eZero,  // dstAlphaBlendFactor
							vk::BlendOp::eAdd,       // alphaBlendOp
							vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
								vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA  // colorWriteMask
						},
					}.data(),
					array<float,4>{0.f,0.f,0.f,0.f}  // blendConstants
				} : blend,

				dynamic == nullptr ? nullptr : dynamic,  // pDynamicState
				pipLay,  // layout
				rp,  // renderPass
				subp == UINT32_MAX ? 0 : subp,  // subpass
				baseHandle == vk::Pipeline(nullptr) ? vk::Pipeline(nullptr) : baseHandle,  // basePipelineHandle
				baseIdx == UINT32_MAX ? -1 : baseIdx // basePipelineIndex
			)
		).value;
		cout << "createPipeline(): Pipeline is created.\n";
	
	return pipe;
}


vk::PipelineLayout HdaPipeline::createPipelineLayout(vk::PipelineLayoutCreateFlags flags, uint32_t layCnt, const vk::DescriptorSetLayout* descrLay, uint32_t pushConRangeCnt,
													 const vk::PushConstantRange* puConRanges) {
	// Prepared for the use of push constants
	vk::PushConstantRange push_const = { vk::ShaderStageFlagBits::eVertex, 0, sizeof(HdaModel::SkyboxPushConstants) };

	vk::PipelineLayout pipeLay;
	pipeLay =
		device.getDevice().createPipelineLayout(
			vk::PipelineLayoutCreateInfo{
				flags,
				layCnt == UINT32_MAX ? 1 : layCnt,       // setLayoutCount
				descrLay == nullptr ? &descriptorSetLayout : descrLay, // pSetLayouts
				pushConRangeCnt == UINT32_MAX ? 1 : pushConRangeCnt,       // pushConstantRangeCount
				puConRanges == nullptr ? &push_const : puConRanges  // pPushConstantRanges
			}
	);
	cout << "createPipelineLayout(): Pipeline layout is created.\n";

	return pipeLay;
}


void HdaPipeline::createShaderModules() {

	vertexShaderModule =
		device.getDevice().createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),
				sizeof(vertexShaderSpirv),  // codeSize
				vertexShaderSpirv  // pCode
			)
		);
	fragmentShaderModule =
		device.getDevice().createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),
				sizeof(fragmentShaderSpirv),  // codeSize
				fragmentShaderSpirv  // pCode
			)
		);

	skyboxVertexShaderModule = 
		device.getDevice().createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),
				sizeof(skyboxVertexShaderSpirv),  // codeSize
				skyboxVertexShaderSpirv  // pCode
			)
		);

	skyboxFragmentShaderModule =
		device.getDevice().createShaderModule(
			vk::ShaderModuleCreateInfo(
				vk::ShaderModuleCreateFlags(),
				sizeof(skyboxFragmentShaderSpirv),  // codeSize
				skyboxFragmentShaderSpirv  // pCode
			)
		);

}