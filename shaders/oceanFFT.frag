#version 430

in vec2 uv;
in vec3 pos;
in float depth;
out vec4 FragColor;

#define PI 3.14159265358979323846

uniform int _TextureZ;
uniform vec3 _lightDir;   
uniform vec3 cameraPos;

uniform vec3 _SunIrradiance = vec3(1.0, 0.694, 0.32);  // Warm white-yellow sunlight.
uniform vec3 _ScatterColor = vec3(0.016, 0.07359998, 0.16);    // Subtle blueish scatter.
uniform vec3 _BubbleColor = vec3(0, 0.02, 0.016);     // Bubble color.
uniform vec3 _FoamColor = vec3(0.9, 0.9, 1.0);       // Light, foamy color.

uniform float _NormalStrength = 1.0;
uniform float _Roughness = 0.075;
uniform float _FoamRoughnessModifier = 0.0;
uniform float _EnvironmentLightStrength = 0.5;
uniform float _HeightModifier = 1.0;
uniform float _BubbleDensity = 1.0;
uniform float _WavePeakScatterStrength = 1.0;
uniform float _ScatterStrength = 1.0;
uniform float _ScatterShadowStrength = 0.5;
uniform float _DisplacementDepthAttenuation = 1.0;
uniform float _UnderwaterFadeStrength=2;
uniform mat4 inverse_model;
uniform samplerCube _EnvironmentMap;
uniform sampler2DArray _DisplacementTextures;  
uniform sampler2DArray _SlopeTextures;
uniform sampler2D _SceneColor;

// Smith masking using the Beckmann distribution
float SmithMaskingBeckmann(vec3 H, vec3 S, float roughness) {
    float hdots = max(0.001, clamp(dot(H, S), 0.0, 1.0));
    float a = hdots / (roughness * sqrt(1.0 - hdots * hdots));
    float a2 = a * a;
    return a < 1.6 ? (1.0 - 1.259 * a + 0.396 * a2) / (3.535 * a + 2.181 * a2) : 0.0;
}

// Beckmann distribution function
float Beckmann(float ndoth, float roughness) {
    float exp_arg = (ndoth * ndoth - 1.0) / (roughness * roughness * ndoth * ndoth);
    return exp(exp_arg) / (PI * roughness * roughness * ndoth * ndoth * ndoth * ndoth);
}

void main() {
    // Normalize light and view directions.
    vec3 lightDir = -normalize(_lightDir);
    vec3 viewDir = normalize(cameraPos - pos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    vec4 displacementFoam;
    for(int i=0;i<_TextureZ;++i){
   displacementFoam += textureLod(_DisplacementTextures, vec3(uv, i), 0.0);
    }
    vec2 slopes ;
     for(int i=0;i<_TextureZ;++i){
    
 slopes += textureLod(_SlopeTextures, vec3(uv, 0), 0.0).rg;
    }
    slopes *= _NormalStrength;

    // Compute foam factor using depth attenuation.
    float foam = clamp(pow(clamp(depth, 0.0, 1.0), _DisplacementDepthAttenuation) * displacementFoam.a, 0.0, 1.0);

    // Compute normals: blend macro (flat) and meso (detailed) normals.
    vec3 macroNormal = vec3(0.0, 1.0, 0.0);
    vec3 mesoNormal = normalize(vec3(-slopes.x, 1.0, -slopes.y));
    mesoNormal = normalize(mix(macroNormal, mesoNormal, pow(clamp(depth, 0.0, 1.0), _DisplacementDepthAttenuation)));
  //  mesoNormal = normalize(mat3(inverse_model) * mesoNormal);

    // Lighting calculations.
    float NdotL = clamp(dot(mesoNormal, lightDir), 0.0, 1.0);
    float a = _Roughness + foam * _FoamRoughnessModifier;
    float ndoth = max(0.0001, dot(mesoNormal, halfwayDir));
    float viewMask = SmithMaskingBeckmann(halfwayDir, viewDir, a);
    float lightMask = SmithMaskingBeckmann(halfwayDir, lightDir, a);
    float G = 1.0 / (1.0 + viewMask + lightMask);

    // Fresnel term using Schlick�s approximation.
    float eta = 1.33;
    float R = ((eta - 1) * (eta - 1)) / ((eta + 1) * (eta + 1));
    float numerator = pow(1 - dot(mesoNormal, viewDir), 5 * exp(-2.69 * a));
  	float F = R + (1 - R) * numerator / (1.0f + 22.7f * pow(a, 1.5f));
    F = clamp(F, 0.0, 1.0);

    // Specular reflection.
    vec3 specular = _SunIrradiance * F * G * Beckmann(ndoth, a);
    specular /= 4.0 * max(0.001, clamp(dot(macroNormal, lightDir), 0.0, 1.0));
    specular *= clamp(dot(macroNormal, lightDir), 0.0, 1.0);

    // Environment reflection.
    vec3 envReflection = texture(_EnvironmentMap, reflect(-viewDir, mesoNormal)).rgb;
    envReflection *= _EnvironmentLightStrength;

    // Wave peak height for scattering calculations.
    float H = max(0.0, displacementFoam.y) * _HeightModifier;

    // Scatter and bubble contributions.
    float k1 = _WavePeakScatterStrength * H * pow(clamp(dot(lightDir, -viewDir), 0.0, 1.0), 4.0) * pow(0.5 - 0.5 * dot(lightDir, mesoNormal), 3.0);
    float k2 = _ScatterStrength * pow(clamp(dot(viewDir, mesoNormal), 0.0, 1.0), 2.0);
    float k3 = _ScatterShadowStrength * NdotL;
    float k4 = _BubbleDensity;
    vec3 scatter = (k1 + k2) * _ScatterColor * _SunIrradiance*(1 / (1.0 + lightMask));
    scatter += k3 * _ScatterColor * _SunIrradiance + k4 * _BubbleColor * _SunIrradiance;

    // Combine specular, environment, and scattering contributions.
    vec3 colorOutput = (1-F)*scatter+specular+envReflection*F ;

    colorOutput = max(vec3(0.0), colorOutput);

    colorOutput = mix(colorOutput, _FoamColor, clamp(foam, 0.0, 1.0));

   





    FragColor = vec4(colorOutput, 1.0);
}



