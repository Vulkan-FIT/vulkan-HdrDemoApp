#pragma once
#include "hda_instancegpu.hpp"
#include "hda_swapchain.hpp"

/*
*
* A class representing vulkan object - pipeline.
*
*/

class HdaPipeline {

public:

	HdaPipeline(HdaInstanceGpu&, HdaSwapchain&);
	~HdaPipeline();

	inline vk::Pipeline getPipeline() { return pipeline; }
	inline vk::PipelineLayout getPipelineLayout() { return pipelineLayout; }
	inline vk::DescriptorSetLayout getDescriptorSetLayout() { return descriptorSetLayout;  }
	inline vk::ShaderModule getSkyboxVertexShaderModule() { return skyboxVertexShaderModule; }
	inline vk::ShaderModule getSkyboxFragmentShaderModule() { return skyboxFragmentShaderModule; }

	void initPipeline();
	void cleanupPipeline();
	vk::Pipeline createPipeline(vk::PipelineCreateFlags flags, uint32_t stgCnt, const vk::PipelineShaderStageCreateInfo* shaders, const vk::PipelineVertexInputStateCreateInfo* vrtxIn,
		const vk::PipelineInputAssemblyStateCreateInfo* assemIn, const vk::PipelineTessellationStateCreateInfo* tess, const vk::PipelineViewportStateCreateInfo* viewPort,
		const vk::PipelineRasterizationStateCreateInfo* raster, const vk::PipelineMultisampleStateCreateInfo* ms, const vk::PipelineDepthStencilStateCreateInfo* depth,
		const vk::PipelineColorBlendStateCreateInfo* blend, const vk::PipelineDynamicStateCreateInfo* dynamic, vk::PipelineLayout pipLay, vk::RenderPass rp, uint32_t subp,
		vk::Pipeline baseHandle, uint32_t baseIdx);
	vk::PipelineLayout createPipelineLayout(vk::PipelineLayoutCreateFlags flags, uint32_t layCnt, const vk::DescriptorSetLayout* descrLay, uint32_t pushConRangeCnt,
		const vk::PushConstantRange* puConRanges);
	vk::DescriptorSetLayout createDescriptorSetLayout(int, uint32_t);

private:

	HdaInstanceGpu& device;
	HdaSwapchain& swapchain;

	int eCh = 0;
	
	// TODO TODO presunout sem funkce z public
	void createShaderModules();

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline pipeline;
	vk::ShaderModule vertexShaderModule;
	vk::ShaderModule fragmentShaderModule;
	vk::ShaderModule skyboxVertexShaderModule;
	vk::ShaderModule skyboxFragmentShaderModule;
	vk::ShaderModule hdrVertexShaderModule;
	vk::ShaderModule hdrFragmentShaderModule;

};