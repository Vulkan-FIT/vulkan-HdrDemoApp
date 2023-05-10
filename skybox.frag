#version 450

layout( location = 0 ) in vec3 vert_texcoord;

layout( binding = 3 ) uniform samplerCube Cubemap;

layout( location = 0 ) out vec4 frag_color;

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


//
// HDR METHODS
//
vec3 reinhardTMO(vec3 result, float e);
vec3 reinhardModTMO(vec3 result, float e);
vec3 hejlDawsonTMO(vec3 result, float e);
vec3 uncharted2TMO(vec3 x);
vec3 originalAcesTMO(vec3 result);

float A = 0.15f;
float B = 0.50f;
float C = 0.10f;
float D = 0.20f;
float E = 0.02f;
float F = 0.30f;
float W = 11.2f;




void main() {


  vec3 hdrResult = texture( Cubemap, vert_texcoord ).rgb;


  //
  // HDR FUNCTION ON
  //
  if (scenedata.hdrOnFlag == 1) {
        
        if (scenedata.chooseMethodFlag == 0)
            hdrResult = reinhardTMO(hdrResult, scenedata.exposure);
        else if (scenedata.chooseMethodFlag == 1)
            hdrResult = hejlDawsonTMO(hdrResult, scenedata.exposure);
        else if (scenedata.chooseMethodFlag == 2) {
        
            hdrResult = hdrResult * scenedata.exposure;
            float exposureBias = 2.0f;
            vec3 curr = uncharted2TMO(exposureBias * hdrResult);

            vec3 whiteScale = vec3(1.0f) / uncharted2TMO(vec3(W));
            hdrResult = curr * whiteScale;
        }
        else if (scenedata.chooseMethodFlag == 3)
            hdrResult = originalAcesTMO(hdrResult * scenedata.exposure);
        else if (scenedata.chooseMethodFlag == 4)
            hdrResult = reinhardModTMO(hdrResult, scenedata.exposure);
    }



    frag_color = vec4(hdrResult, 1.0);
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
   vec3 retResult = (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
   
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
