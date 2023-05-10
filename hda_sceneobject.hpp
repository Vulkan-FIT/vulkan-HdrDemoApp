#pragma once
#include "hda_model.hpp"


class HdaSceneObject {

public:

	HdaSceneObject();
	~HdaSceneObject();


	void createSceneObject(HdaModel::Mesh, std::vector<HdaModel::Texture>, glm::mat4, 
						   vk::Pipeline, vk::PipelineLayout, std::vector<vk::DescriptorSet>, vk::DescriptorSetLayout, uint32_t);

private:

	HdaModel::Mesh objMesh;

	std::vector<HdaModel::Texture> objTexture;
	
	int nonTextureFlag = 0;
	int nonRotateFlag = 0;
	
	glm::mat4 modelMatrix = glm::mat4{ 1.0f };
	
	vk::Pipeline objPipeline;
	vk::PipelineLayout objPipelineLayout;
	std::vector<vk::DescriptorSet> objDescriptSets;
	vk::DescriptorSetLayout objDescriptSetLay;

	uint32_t objIndex = UINT32_MAX;
};