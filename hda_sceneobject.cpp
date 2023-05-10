#include "hda_sceneobject.hpp"

HdaSceneObject::HdaSceneObject() {

	std::cout << "HdaSceneObject(): constructor\n";
}

HdaSceneObject::~HdaSceneObject() {

	std::cout << "HdaSceneObject():Destructor\n";
}


void HdaSceneObject::createSceneObject(HdaModel::Mesh mesh, std::vector<HdaModel::Texture> texture, glm::mat4 modelMat,
	vk::Pipeline pipe, vk::PipelineLayout pipeLay, std::vector<vk::DescriptorSet> descSets, vk::DescriptorSetLayout descLay, uint32_t idx) {

	objMesh = mesh;
	objTexture = texture;
	modelMatrix = modelMat;
	objPipeline = pipe;
	objPipelineLayout = pipeLay;
	objDescriptSets = descSets;
	objDescriptSetLay = descLay;
	objIndex = idx;
}