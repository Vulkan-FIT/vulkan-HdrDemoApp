#pragma once
#include "hda_window.hpp"

#include "vulkan/vulkan.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE	// depth range convert for Vulkan usage 0.0 - 1.0
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <chrono>
#include <iomanip>
#include <unordered_map>

#define P (std::cout << "print debug" << endl)

class HdaModel {

public:
	
	//////// structures
	
	struct Vertex {

		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 color;
		glm::vec2 uv;

		// binding description
		static vk::VertexInputBindingDescription getBindingDescription();

		// attribute description
		static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescription();

		bool operator==(const Vertex& other) const {
			return position == other.position && color == other.color && uv == other.uv;
		}
	};

	struct IndexInfo {

		uint32_t vertexCnt = 0;		// vertex count;
		uint32_t firstVertex = 0;	// offset for drawing - where to start drawing
		uint32_t textureIndex = UINT32_MAX;	// index for texture
	};

	struct Material {

		glm::vec4 ambient{ 1.0f };
		glm::vec4 diffuse{ 1.0f };
		glm::vec4 specular{ 1.0f };
		float shi = 16;
	};

	struct Mesh {

		std::string basedir = "..\\models";

		std::vector<Vertex> meshVertices;
		std::vector<uint32_t> meshIndices;

		vk::Buffer vertexBuff;
		vk::DeviceMemory vertexBuffMemory;
		vk::Buffer indexBuff;
		vk::DeviceMemory indexBuffMemory;

		// multiMesh data
		std::vector<std::string> texNames;
		uint32_t numMat = 0;
		std::vector<IndexInfo> info;
		std::vector<Material> mats;
		uint32_t submeshCnt = 0;

		void loadObjFormat(const char*, std::string);
	};

	struct PushConstants {

		glm::vec3 cameraPosition;
		int texId;
		glm::mat4 modelMatrix;
	};

	struct SkyboxPushConstants {

		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
	};

	struct ProjectionUniformData {

		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct SceneUniformData {

		alignas(16) glm::mat4 normalMatrix{ 1.0f };
		alignas(16) glm::mat4 normalMatrixWorld{ 1.0f };
		alignas(16) glm::mat4 modelView{ 1.0f };
		int nontextureFlag = 0;
		int blinnPhongFlag = 0;
		int pointLightFlag = 0;
		int hdrOnFlag = 0;
		alignas(4) float exposure = 1.0f;
		int chooseMethodFlag = 0;
	};

	struct MaterialLightUniformData {

		// material data
		alignas(16) glm::vec4 materialAmbient{ 1.0f };
		alignas(16) glm::vec4 materialDiffuse{ 1.0f };
		alignas(16) glm::vec4 materialSpecular{ 1.0f };

		// light data
		alignas(16) std::array<glm::vec4, 2> lightPositions;
		alignas(16) std::array<glm::vec4, 2> lightDirections;
		alignas(16) glm::vec4 lightAmbient{ 1.0f };
		alignas(16) glm::vec4 lightDiffuse{ 1.0f };
		alignas(16) glm::vec4 lightSpecular{ 1.0f };

		alignas(4) float materialShininess = 16.0f;

		alignas(4) float kC = 1.0f;
		alignas(4) float kL = 0.007f;		// 0.045
		alignas(4) float kQ = 0.0002f;		// 0.0075

		alignas(4) float cutOff = glm::cos(glm::radians(15.5f));
		alignas(4) float outerCutOff = glm::cos(glm::radians(20.5f));
	};

	struct Texture {

		vk::Image textureImage;
		vk::DeviceMemory textureImageMemory;
		vk::ImageView textureImageView;
		vk::Sampler textureSampler;
	};

	struct SceneObject {

		Mesh objectMesh;
		std::vector<Texture> objectTexture;
		int nonTextureFlag = 0;
		int nonRotateFlag = 0;
		int texObjIndex = 0;
		int multiTextureFlag = 0;
		glm::mat4 modelMatrix = glm::mat4{ 1.0f };
		vk::Pipeline objectPipeline;
		vk::PipelineLayout objectPipelineLayout;
		std::vector<vk::DescriptorSet> objectDescriptSets;
		std::vector<std::vector<vk::DescriptorSet>> dsv;
		vk::DescriptorSetLayout objectDescriptSetLay;
	};

	//////// functions

	static ProjectionUniformData projectionCalculation(glm::mat4, GLFWwindow*, glm::mat4*, glm::vec3, HdaModel::SceneUniformData*, bool, float*);
	static void HdaModel::loadMaterialData(HdaModel::MaterialLightUniformData*, glm::vec4, glm::vec4, glm::vec4, float);
	static void HdaModel::loadLightData(HdaModel::MaterialLightUniformData*, glm::vec4, glm::vec4, glm::vec4, std::array<glm::vec4, 2>, std::array<glm::vec4, 2>);
};

namespace std {

	template<> struct hash<HdaModel::Vertex> {

		size_t operator()(HdaModel::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}