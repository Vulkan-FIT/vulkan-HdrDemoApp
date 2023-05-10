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

layout(binding = 1) uniform SceneUniformData {

    mat4 normalMatrix;
    mat4 normalMatrixWorld;
    mat4 modelViewMatrix;
    int nontextureFlag;
    int blinnPhongFlag;
    int pointLightFlag;
    int hdrOnFlag;

} scenedata;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexture;
layout(location = 2) out vec3 fragPos;  
layout(location = 3) out vec3 outNormal; 
layout(location = 4) out vec3 outView; 
layout(location = 5) out vec3 outLight;
layout(location = 6) out vec3 outNormalWorld;
layout(location = 7) out vec3 fragPosWorld; 
layout(location = 8) out flat int tId; 

layout( push_constant ) uniform constants {

    vec3 cameraPosition;
	int texId;
	mat4 modelMatrix;

} PushConstants;

const vec4 LIGHT_RAY_POSITION = vec4(8.0f, 2.0f, 11.0f, 0.0f);

void main() {

    mat4 transformationMatrix = uniformProjection.proj * uniformProjection.view * PushConstants.modelMatrix;
    gl_Position = transformationMatrix * vec4(inPosition, 1.0);
   
    fragTexture = inTexture;
    fragColor = inColor;
    fragPos = vec3(scenedata.modelViewMatrix * vec4(inPosition, 1.0));
    fragPosWorld =  vec3(PushConstants.modelMatrix * vec4(inPosition, 1.0));                // CHANGED
    outNormal = mat3(scenedata.normalMatrix) * inNormal;
    outLight = vec3(uniformProjection.view * LIGHT_RAY_POSITION);
    outView = PushConstants.cameraPosition;
    outNormalWorld = mat3(scenedata.normalMatrixWorld) * inNormal;    // CHANGED
    tId = PushConstants.texId;
}
