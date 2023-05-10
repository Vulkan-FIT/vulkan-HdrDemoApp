#version 450

//
// in/out vectors
//
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexture;
layout(location = 2) in vec3 fragPos;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inView;
layout(location = 5) in vec3 inLight;
layout(location = 6) in vec3 inNormalWorld;
layout(location = 7) in vec3 fragPosWorld;
layout(location = 8) in flat int texId;

layout(location = 0) out vec4 outColor;

//
// Uniform structures
//
layout(binding = 1) uniform SceneUniformData {

    mat4 normalMatrix;
    mat4 normalMatrixWorld;
    mat4 modelViewMatrix;
    int nontextureFlag;
    int blinnPhongFlag;
    int pointLightFlag;
    int hdrOnFlag;
    float exposure;
    int chooseMethodFlag;

} scenedata;

layout(binding = 2) uniform MaterialLightUniformData {

    // material data
	vec4 materialAmbient;
	vec4 materialDiffuse;
	vec4 materialSpecular;
	
	// light data
    vec4 lightPositions[2];
    vec4 lightDirections[2];
	vec4 lightAmbient;
	vec4 lightDiffuse;
	vec4 lightSpecular;

    float materialShininess;

    // attenuation coeficients
    float kC;
    float kL;
    float kQ;

    // spotlight
    float cutOff;
    float outerCutOff;

} mlData;

//
// texture sampler
//
layout(binding = 3) uniform sampler2D texSampler;

//
// constants
//
const vec3 LIGHT_COLOR = vec3(1.0, 0.985, 0.990);
const float AMBIENT = 0.08;
const float SPECULAR_STRENGTH = 0.7;
const float SHININESS = 32;
const int NUM_OF_POINTLIGHTS = 2;

//
// function prototypes
//
vec3 CalcDirectionalLight(vec3 normal, vec3 viewDir, vec3 fragP);
vec3 CalcPointLight(vec3 normal, vec3 viewDir, vec3 fragP, vec4 lightP, vec4 lightD);

vec3 reinhardTMO(vec3 result, float e);
vec3 reinhardModTMO(vec3 result, float e);
vec3 hejlDawsonTMO(vec3 result, float e);
vec3 uncharted2TMO(vec3 x);
vec3 originalAcesTMO(vec3 result);

// HDR PART
const float gamma = 2.2;

float A = 0.15f;
float B = 0.50f;
float C = 0.10f;
float D = 0.20f;
float E = 0.02f;
float F = 0.30f;
float W = 11.2f;

void main() {

    // result of hdr tone mapping
    vec3 hdrResult;

    // in view space
    vec3 norm = normalize(inNormal);
    vec3 viewDir = normalize(inView - fragPos);

    // in world space
    vec3 normWorld = normalize(inNormalWorld);
    vec3 viewDirWorld = normalize(inView - fragPosWorld);

    // directional lighting
    vec3 result = CalcDirectionalLight(norm, viewDir, fragPos) * fragColor;

    // point lights 
    int i = 0;       
    //for(int i = 0; i < 1; i++)
    if (scenedata.pointLightFlag == 1) {
        result += CalcPointLight(normWorld, viewDirWorld, fragPosWorld, mlData.lightPositions[i], mlData.lightDirections[i++]);
        result += CalcPointLight(normWorld, viewDirWorld, fragPosWorld, mlData.lightPositions[i], mlData.lightDirections[i++]);
    }


    if (scenedata.nontextureFlag == 0)    
        result = result * texture(texSampler, fragTexture).rgb;

    if (scenedata.hdrOnFlag == 1) {
        
        if (scenedata.chooseMethodFlag == 0)
            hdrResult = reinhardTMO(result, scenedata.exposure);
        else if (scenedata.chooseMethodFlag == 1)
            hdrResult = hejlDawsonTMO(result, scenedata.exposure);
        else if (scenedata.chooseMethodFlag == 2) {
        
            result = result * scenedata.exposure;
            float exposureBias = 2.0f;
            vec3 curr = uncharted2TMO(exposureBias * result);

            vec3 whiteScale = vec3(1.0f) / uncharted2TMO(vec3(W));
            hdrResult = curr * whiteScale;
        }
        else if (scenedata.chooseMethodFlag == 3)
            hdrResult = originalAcesTMO(result * scenedata.exposure);
        else if (scenedata.chooseMethodFlag == 4)
            hdrResult = reinhardModTMO(result, scenedata.exposure);

        outColor = vec4(hdrResult, 1.0);
    }
    else {

        outColor = vec4(result, 1.0);
    }
}


// TODO TODO upravit komentare k funkcim
/**
 *   Code of direction lights, point lights and Phong lighting modified from 
 *   tutorial article obtained from LearnOpenGL:
 *     https://learnopengl.com/Lighting/Basic-Lighting
 */
vec3 CalcDirectionalLight(vec3 normal, vec3 viewDir, vec3 fragP) {
    
    // ambient lighting
     vec3 ambient = vec3(mlData.lightAmbient) * vec3(mlData.materialAmbient); 
    
    // diffuse lighting
    vec3 lightDir = normalize(vec3(inLight));

    // Lambertian cosine law
    float diff = max(dot(normal, lightDir), 0.0); // if dot product of vectors is greater then 90 degrees, result is negative
                                                  // so max of these two values return values between 0 and 1
   
    float spec = 0.0;
    if (diff > 0.0) {

        if (scenedata.blinnPhongFlag == 1) {

            // Blinn-Phong
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float s = max(dot(normal, halfwayDir), 0.0);
            if (s > 0.0)
                spec = pow(s, mlData.materialShininess); //16.0);   CHANGED TODO TODO
        } 
        else {


            // Phong
            vec3 reflectDir = reflect(-lightDir, normal);
            float s = max(dot(viewDir, reflectDir), 0.0);
            if (s > 0.0)
                spec = pow(s, mlData.materialShininess); //8.0);  CHANGED
        }
    }
    
    vec3 diffuse  = (diff * vec3(mlData.materialDiffuse)) * vec3(mlData.lightDiffuse);
    vec3 specular = (spec * vec3(mlData.materialSpecular)) * vec3(mlData.lightSpecular);

    return (ambient + diffuse + specular);
}

vec3 CalcPointLight(vec3 normal, vec3 viewDir, vec3 fragP, vec4 lightP, vec4 lightD) {
    
    // the vector pointing from the fragment to the light source
    vec3 lightDirToFrag = normalize(vec3(lightP) - fragP);

    // ambient lighting
    vec3 ambient = vec3(mlData.lightAmbient) * vec3(mlData.materialAmbient);

    // check if lighting is inside the spotlight cone (lightDirection is the direction the spotlight is aiming at)
    float theta = dot(lightDirToFrag, normalize(vec3(-lightD)));

    //if(theta > mlData.cutOff) {

        // diffuse lighting
        // Lambertian cosine law
        float diff = max(dot(normal, lightDirToFrag), 0.0); // if dot product of vectors is greater then 90 degrees, result is negative
                                                            // so max of these two values return values between 0 and 1
   
        float spec = 0.0;
        if (diff > 0.0) {
        
            if (scenedata.blinnPhongFlag == 1) {
    
                // Blinn-Phong
                vec3 halfwayDir = normalize(lightDirToFrag + viewDir);
                float s = max(dot(normal, halfwayDir), 0.0);
                if (s > 0.0)
                    spec = pow(s, mlData.materialShininess); //32.0); //mlData.materialShininess);  
            }
            else {
    
                // Phong
                vec3 reflectDir = reflect(-lightDirToFrag, normal); // reflected light
                float s = max(dot(viewDir, reflectDir), 0.0);
                if (s > 0.0)
                    spec = pow(s, mlData.materialShininess); //32.0); //mlData.materialShininess);
            }
        }
    
        vec3 diffuse  = (diff * vec3(mlData.materialDiffuse)) * vec3(mlData.lightDiffuse);
        vec3 specular = (spec * vec3(mlData.materialSpecular)) * vec3(mlData.lightSpecular);

        // spotlight soft edges
        float epsilon = mlData.cutOff - mlData.outerCutOff;
        float intensity = max(0.0f, min((theta - mlData.outerCutOff)/epsilon, 1.0f));
        diffuse  *= intensity;
        specular *= intensity;

        // attenuation for point light
        float distance = length(vec3(lightP) - fragP);
        float attenuation = 1.0 / (mlData.kC + mlData.kL * distance + mlData.kQ * (distance * distance));



        //ambient *= attenuation;  // as otherwise at large distances the light would be darker inside than outside the spotlight due the ambient term in the else branch
        diffuse *= attenuation;
        specular *= attenuation;

        return (diffuse + specular);
}

//
//  TONE MAPPING FUNCTIONS
//
vec3 reinhardTMO(vec3 result, float e) {

    result *= e; 
    vec3 hdrResult = result / (1.0f + result);  // TODO TODO add modRein with Lw

    return hdrResult;
}


vec3 reinhardModTMO(vec3 result, float e) {

    vec3 hdrResult = vec3(1.0f) - exp(-result * e);

    return hdrResult;
}


vec3 hejlDawsonTMO(vec3 result, float e) {

   vec3 x = max(((result * e) - 0.004f), 0.0);
   vec3 retResult = pow((x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f), vec3(2.2));
   //vec3 retResult = (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);

   return retResult;
}


vec3 uncharted2TMO(vec3 x) {

   return ((x * (A * x + C * B) + D * E)/(x * (A * x + B) + D * F)) - E/F;
}


// Based on http://www.oscars.org/science-technology/sci-tech-projects/aces
vec3 originalAcesTMO(vec3 result) {	

	mat3 m1 = mat3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
	);

	mat3 m2 = mat3(
        1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
	);

	vec3 v = m1 * result;    
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;

	//return pow(clamp(m2 * (a / b), 0.0, 1.0), vec3(1.0 / 2.2));	// UVNITR ZAKOMPONOVANA I GAMA KOREKCE KTERA JE NEZADOUCI
    return clamp(m2 * (a / b), 0.0, 1.0);	
}