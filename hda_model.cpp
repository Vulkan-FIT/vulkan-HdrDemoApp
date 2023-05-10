#include "hda_model.hpp"


#define TINYOBJLOADER_IMPLEMENTATION
#include "external/include/tiny_obj_loader.h"

#include <iostream>

#define VIEW_SPEED float(0.21f)		// 0.11
#define MOVE_SPEED float(0.05f)

float xx = 0, yy = 1, zz = 0;
int width, height;
float viewSpeed = 0.11f;
float moveSpeed = 0.05f;

glm::vec3 camera = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 zCameraSpace = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 yCameraSpace = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 xCameraSpace = glm::vec3(1.0f, 0.0f, 0.0f);


vk::VertexInputBindingDescription HdaModel::Vertex::getBindingDescription() {

	return{ 0,			// one buffer for all vetex attributes - interleaved implementation
		sizeof(Vertex), //  the step to move to the next data item, i.e. to the next vertex
		vk::VertexInputRate::eVertex // per vertex (equivalent is per instance)
	};
}

std::array<vk::VertexInputAttributeDescription, 4> HdaModel::Vertex::getAttributeDescription() {

	std::array<vk::VertexInputAttributeDescription, 4> attributeDescription;

	// position
	attributeDescription[0].binding = 0;
	attributeDescription[0].location = 0;
	attributeDescription[0].format = vk::Format::eR32G32B32Sfloat;
	attributeDescription[0].offset = offsetof(Vertex, position);  // offsetof() - stddef.h

	// normal
	attributeDescription[1].binding = 0;
	attributeDescription[1].location = 1;
	attributeDescription[1].format = vk::Format::eR32G32B32Sfloat;
	attributeDescription[1].offset = offsetof(Vertex, normal);

	// color
	attributeDescription[2].binding = 0;
	attributeDescription[2].location = 2;
	attributeDescription[2].format = vk::Format::eR32G32B32Sfloat;
	attributeDescription[2].offset = offsetof(Vertex, color);

	// color
	attributeDescription[3].binding = 0;
	attributeDescription[3].location = 3;
	attributeDescription[3].format = vk::Format::eR32G32Sfloat;
	attributeDescription[3].offset = offsetof(Vertex, uv);

	return attributeDescription;
}


/*
*  Code of attribute loading modified from example on official github page tinyobjloader:
*  https://github.com/tinyobjloader/tinyobjloader
* 
*/
void HdaModel::Mesh::loadObjFormat(const char* filename, std::string mtlBasedir) {

	// TODO TODO pouzit c++ tinyobjloader
	// TODO TODO prepsat nazvy pronennych na camelstyle

	//attrib will contain the vertex arrays of the file
	tinyobj::attrib_t attrib;
	//shapes contains the info for each separate object in the file
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	//error and warning output from the load function
	std::string warn;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, mtlBasedir.c_str())) { //basedir.c_str())) {
		throw std::runtime_error(err);
	}

	if (!warn.empty())
		std::cout << std::endl << "[WARNING]<tinyobjloader> Loadobj(): " << warn << std::endl;
	
	//auto tI = 0; // first free slot in texture array  = textureIndex
	uint32_t vertexCount = 0; // the number of indices to be drawn in one bundle
	auto fstVertex = static_cast<uint32_t>(meshVertices.size()); // index offset for drawing
	int currentMat = 0; // the current OBJ material being used in face loop
	int submeshCount = 0;	// number of subgroup of triangles, which will be rendered
	float i = 0.0f;

	// Loading textures and material properties from .mtl
	for (const auto & mat : materials) {

		texNames.push_back(mat.diffuse_texname);
		
		Material m;
		m.ambient.x = mat.ambient[0]; m.ambient.y = mat.ambient[1]; m.ambient.z = mat.ambient[2]; m.ambient.w = 1.0f;
		m.diffuse.x = mat.diffuse[0]; m.diffuse.y = mat.diffuse[1]; m.diffuse.z = mat.diffuse[2]; m.diffuse.w = 1.0f;
		m.specular.x = mat.specular[0]; m.specular.y = mat.specular[1]; m.specular.z = mat.specular[2]; m.specular.w = 1.0f;
		m.shi = mat.shininess;
		mats.push_back(m);

		numMat++;
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {	// TODO TODO jak se to prochází

			auto thisMat = shapes[s].mesh.material_ids[f];
			if (thisMat != currentMat) {

				submeshCount++;
				
				// load data to structure
				IndexInfo inf;
				inf.vertexCnt = vertexCount;
				inf.firstVertex = fstVertex;
				inf.textureIndex = currentMat;
				info.push_back(inf);

				// restart variables for new data
				currentMat = thisMat;
				fstVertex = meshVertices.size(); // fstVertex + vertexCount;	// TODO TODO TOTO TAKY FUNGUJE
				vertexCount = 0;
			}


			// triangles -> num_face_vertices = 3
			int fv = 3;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				//tinyobj::index_t mI = shapes[s].mesh.material_ids[f];

				Vertex new_vert{};						// TODO TODO poresit tady to pretypovani co sa samo nabizi, proc to tak ma byt
				new_vert.position.x = attrib.vertices[static_cast<std::vector<tinyobj::real_t, std::allocator<tinyobj::real_t>>::size_type>(3) * idx.vertex_index + 0];
				new_vert.position.y = attrib.vertices[static_cast<std::vector<tinyobj::real_t, std::allocator<tinyobj::real_t>>::size_type>(3) * idx.vertex_index + 1];
				new_vert.position.z = attrib.vertices[static_cast<std::vector<tinyobj::real_t, std::allocator<tinyobj::real_t>>::size_type>(3) * idx.vertex_index + 2];

				// vertex normals
				new_vert.normal.x = attrib.normals[3 * idx.normal_index + 0];
				new_vert.normal.y = attrib.normals[3 * idx.normal_index + 1];
				new_vert.normal.z = attrib.normals[3 * idx.normal_index + 2];

				// vertex colors
				new_vert.color.x = attrib.colors[3 * idx.vertex_index + 0];
				new_vert.color.y = attrib.colors[3 * idx.vertex_index + 1];
				new_vert.color.z = attrib.colors[3 * idx.vertex_index + 2];

				//vertex uv (textures coordinates)
				tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];
				new_vert.uv.x = ux;
				new_vert.uv.y = 1 - uy;

				//new_vert.color = { 1.0f, 0.0f, 0.0f};
				//new_vert.color = new_vert.normal;
				
				// TODO TODO PUVODNI
				meshVertices.push_back(new_vert);

				// TODO if index buffer 
				//meshIndices.push_back(meshVertices.size());

				// ##################################################################
				/*if (uniqueVertices.count(new_vert) == 0) {

					uniqueVertices[new_vert] = static_cast<uint32_t>(meshVertices.size());
					meshVertices.push_back(new_vert);
				}
				meshIndices.push_back(uniqueVertices[new_vert]);*/
				// ################################################################

				vertexCount++;
			}
			index_offset += fv;

			// TODO TODO if index buffer
			//meshIndices.push_back(static_cast<uint32_t>(meshIndices.size()));
		}

		// loading data to structure for last sub-group of vertices
		IndexInfo inf;
		inf.vertexCnt = vertexCount;
		inf.firstVertex = fstVertex;
		info.push_back(inf);
	}

	submeshCnt = submeshCount;
}

// TODO TODO
/*
*  Code of camera movement modified from tutorial article obtained from LearnOpenGL:
*  https://learnopengl.com/Getting-started/Camera
* 
*/
HdaModel::ProjectionUniformData HdaModel::projectionCalculation(glm::mat4 model, GLFWwindow* window, glm::mat4* skyboxView, glm::vec3 camPos, HdaModel::SceneUniformData* scenedata, bool hdrOnFlag, float *exposure) {

	HdaModel::ProjectionUniformData modelviewProjection{};

	xCameraSpace = glm::normalize(glm::cross(zCameraSpace, yCameraSpace));

	// TODO mouse scroll = zoom
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) { camera += zCameraSpace * MOVE_SPEED; }	// forward-backward move
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) { camera -= zCameraSpace * MOVE_SPEED; }

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) { camera -= xCameraSpace * MOVE_SPEED; }	// left-right move
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) { camera += xCameraSpace * MOVE_SPEED; }

	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) { camera -= glm::normalize(glm::cross(zCameraSpace, xCameraSpace)) * MOVE_SPEED; } // up-down move
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) { camera += glm::normalize(glm::cross(zCameraSpace, xCameraSpace)) * MOVE_SPEED; }

	// TODO TODO smazat
	//camera position
	//glm::vec3 camPos = { 0.f, monkeyLoaded == 1 ? 0.0f : -19.f, monkeyLoaded == 1 ? -2.f : -25.f };  // monkeyLoaded = 0 for { 0.f,-9.f,-20.f };

	//camera projection
	glfwGetFramebufferSize(window, &width, &height);
	glm::mat4 projection = glm::perspective(glm::radians(55.f), float(width)/float(height), 0.1f, 400.0f);
	projection[1][1] *= -1;
	modelviewProjection.proj = projection;


	static auto startT = std::chrono::high_resolution_clock::now();
	auto currentT = std::chrono::high_resolution_clock::now();
	float t = std::chrono::duration<float, std::chrono::seconds::period>(currentT - startT).count();    


	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { yy += 1 * VIEW_SPEED; }	// left-right view
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { yy -= 1 * VIEW_SPEED; }
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { xx += 1 * VIEW_SPEED; }	// up-down view
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { xx -= 1 * VIEW_SPEED; }

	// camera view
	// Euler angles 
	// yaw = yy axis     pitch = xx axis	roll = zz axis
	glm::vec3 front = { cos(glm::radians(yy)) * cos(glm::radians(xx)),
						sin(glm::radians(xx)),
						sin(glm::radians(yy))* cos(glm::radians(xx)) };

	zCameraSpace = glm::normalize(front);
	glm::vec3 center = camera + zCameraSpace;
	modelviewProjection.view = glm::lookAt(camera, center, yCameraSpace);

	*skyboxView = glm::mat4(glm::mat3(glm::lookAt(camera, center, yCameraSpace)));

	camPos = camera;
	/*if (skybox == 1) {	// if the skybox is to be rendered, discard the translation part of the matrix			// TODO TODO smazat
		modelviewProjection.view = glm::mat4(glm::mat3(glm::lookAt(camera, center, yCameraSpace)));
	}
	std::cout << sizeof(glm::mat4) << "   ";*/
	
	
	// model rotation
	//modelviewProjection.model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0, 1, 0));	// glm::mat4{ 1.0f }
	//modelviewProjection.model = glm::translate(modelviewProjection.model, { 0, -20, 0 });
	//modelviewProjection.model = glm::rotate(glm::mat4{ 1.0f }, t * glm::radians(40.0f), glm::vec3(0, 1, 0));
	//															rychlost				rotace kolem osy x, kolem y, kolem z
	/*const float* pSource = (const float*)glm::value_ptr(model);
	std::cout << "model data: " << pSource[0];
	std::cout << pSource[1];
	*/

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) { scenedata->blinnPhongFlag = 1; }
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) { scenedata->pointLightFlag = 1; }
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) { *exposure += 0.008f; }
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) { *exposure -= 0.008f; }


	modelviewProjection.model = model;

	// TODO TODO smazat
	//calculate final mesh matrix
	//glm::mat4 mesh_matrix = projection * view * model;

	//HdaModel::PushConstants constants;
	//constants.modelMatrix = model;

	return modelviewProjection;
}


void HdaModel::loadMaterialData(HdaModel::MaterialLightUniformData* mlData, glm::vec4 ambient, glm::vec4 diffuse, glm::vec4 specular, float shininess) {

	mlData->materialAmbient = ambient;
	mlData->materialDiffuse = diffuse;
	mlData->materialSpecular = specular;
	mlData->materialShininess = shininess;
}


void HdaModel::loadLightData(HdaModel::MaterialLightUniformData* mlData, glm::vec4 lightAmbient, glm::vec4 lightDiffuse, glm::vec4 lightSpecular, std::array<glm::vec4, 2> positions, std::array<glm::vec4, 2> directions) {

	mlData->lightAmbient = lightAmbient;
	mlData->lightDiffuse = lightDiffuse;
	mlData->lightSpecular = lightSpecular;

	mlData->lightDirections = directions;
	mlData->lightPositions = positions;
}