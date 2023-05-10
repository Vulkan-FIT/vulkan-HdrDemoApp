#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexture;

layout(binding = 0) uniform ProjectionUniformData {

    mat4 model;
    mat4 view;
    mat4 proj;

} uniformProjection;

layout(location = 0) out vec3 outCube;

layout( push_constant ) uniform constants {

	mat4 viewMatrix;
	mat4 modelMatrix;

} PushConstants;

void main() {

    vec3 position = mat3(PushConstants.modelMatrix) * inPosition;
    gl_Position = (uniformProjection.proj * PushConstants.viewMatrix * vec4( position, 1.0 )).xyww;

    outCube = inPosition;
}