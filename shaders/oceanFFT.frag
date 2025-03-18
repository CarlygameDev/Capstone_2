#version 430


in vec2 uv;
in vec3 pos;
in float depth;
out vec4 FragColor;
#define PI 3.14159265358979323846


uniform vec3 _lightDir;   
uniform vec3 cameraPos;

uniform vec3 _SunIrradiance = vec3(1.0, 0.694, 0.32);  // A warm white-yellow light, typical for sunlight.
uniform vec3 _ScatterColor = vec3(0.016, 0.07359998, 0.16);    // A subtle color for scattering, often blueish.
uniform vec3 _BubbleColor = vec3(0, 0.02, 0.016);     // White color for foam or bubbles.
uniform vec3 _FoamColor = vec3(0.9, 0.9, 1.0);       // Light, foamy color, often light blue/white.


uniform float _NormalStrength=1;
uniform float _Roughness=0.075;
uniform float _FoamRoughnessModifier=0;
uniform float _EnvironmentLightStrength=1;
uniform float _HeightModifier=1;

uniform float _BubbleDensity=1;

uniform float _WavePeakScatterStrength=1;
uniform float _ScatterStrength=1;
uniform float _ScatterShadowStrength=0.5;
uniform float _DisplacementDepthAttenuation=1;


uniform mat4 model;
uniform samplerCube _EnvironmentMap;
uniform sampler2DArray _DisplacementTextures;  
uniform sampler2DArray _SlopeTextures;

float SmithMaskingBeckmann(vec3 H, vec3 S, float roughness) {
				float hdots = max(0.001f, clamp(dot(H, S),0.0,1.0));
				float a = hdots / (roughness * sqrt(1 - hdots * hdots));
				float a2 = a * a;

				return a < 1.6f ? (1.0f - 1.259f * a + 0.396f * a2) / (3.535f * a + 2.181 * a2) : 0.0f;
			}

			float Beckmann(float ndoth, float roughness) {
				float exp_arg = (ndoth * ndoth - 1) / (roughness * roughness * ndoth * ndoth);

				return exp(exp_arg) / (PI * roughness * roughness * ndoth * ndoth * ndoth * ndoth);
			}


void main() {
   vec3 lightDir = -normalize(_lightDir);
   vec3 viewDir = normalize(cameraPos - pos);
   vec3 halfwayDir = normalize(lightDir + viewDir);


   float LdotH = clamp(dot(lightDir, halfwayDir), 0.0, 1.0);
   float VdotH = clamp(dot(viewDir, halfwayDir), 0.0, 1.0);


  vec4 displacementFoam1 = textureLod(_DisplacementTextures, vec3(uv, 0),0 );
  vec4 displacementFoam2 = textureLod(_DisplacementTextures, vec3(uv, 1),0) ;
  vec4 displacementFoam3 = textureLod(_DisplacementTextures,vec3(uv, 2),0 );
  vec4 displacementFoam4 = textureLod(_DisplacementTextures, vec3(uv, 3),0);
  vec4 displacementFoam = displacementFoam1 + displacementFoam2 + displacementFoam3 + displacementFoam4;

   vec2 slopes1 = textureLod(_SlopeTextures, vec3(uv, 0),0 ).rg;
				vec2 slopes2 = textureLod(_SlopeTextures, vec3(uv, 1),0 ).rg;
				vec2 slopes3 = textureLod(_SlopeTextures, vec3(uv, 2),0 ).rg;
				vec2 slopes4 = textureLod(_SlopeTextures, vec3(uv, 3),0 ).rg;
				vec2 slopes = slopes1 + slopes2 + slopes3 + slopes4;

				slopes *= _NormalStrength;

//float foam = mix(0.0f, displacementFoam.a, pow(clamp(depth, 0.0, 1.0), _DisplacementDepthAttenuation));
float foam=displacementFoam.a;

vec3 macroNormal = vec3(0, 1, 0);
				vec3 mesoNormal = normalize(vec3(-slopes.x, 1.0f, -slopes.y));
				mesoNormal = normalize(mix(macroNormal, mesoNormal, pow(clamp(depth, 0.0, 1.0), _DisplacementDepthAttenuation)));

				mesoNormal = normalize(mat3(transpose(inverse(model))) * normalize(mesoNormal));

				float NdotL = clamp(dot(mesoNormal, lightDir),0.0,1.0);

				float a = _Roughness + foam * _FoamRoughnessModifier;
				float ndoth = max(0.0001f, dot(mesoNormal, halfwayDir));

				float viewMask = SmithMaskingBeckmann(halfwayDir, viewDir, a);
				float lightMask = SmithMaskingBeckmann(halfwayDir, lightDir, a);

				float G = 1/(1 + viewMask + lightMask);

				float eta = 1.33f;
				float R = ((eta - 1) * (eta - 1)) / ((eta + 1) * (eta + 1));
				float thetaV = acos(viewDir.y);
                float numerator = pow(1 - dot(mesoNormal, viewDir), 5 * exp(-2.69 * a));
				float F = R + (1 - R) * numerator / (1.0f + 22.7f * pow(a, 1.5f));
				F = clamp(F,0.0,1.0);

				vec3 specular = _SunIrradiance * F * G * Beckmann(ndoth, a);
				specular /= 4.0f * max(0.001f, clamp(dot(macroNormal, lightDir),0.0,1.0));
				specular *= clamp(dot(macroNormal, lightDir),0.0,1.0);

				vec3 envReflection = texture(_EnvironmentMap, reflect(-viewDir, mesoNormal)).rgb;
				envReflection *= _EnvironmentLightStrength;

				float H = max(0.0f, displacementFoam.y) * _HeightModifier;
				vec3 scatterColor = _ScatterColor;
				vec3 bubbleColor = _BubbleColor;
				float bubbleDensity = _BubbleDensity;

				float k1 = _WavePeakScatterStrength * H * pow(clamp(dot(lightDir, -viewDir),0.0,1.0), 4.0f) * pow(0.5f - 0.5f * dot(lightDir, mesoNormal), 3.0f);
				float k2 = _ScatterStrength * pow(clamp(dot(viewDir, mesoNormal),0.0,1.0), 2.0f);
				float k3 = _ScatterShadowStrength * NdotL;
				float k4 = bubbleDensity;

				vec3 scatter = (k1 + k2) * scatterColor * _SunIrradiance * (1/(1 + lightMask));
				scatter += k3 * scatterColor * _SunIrradiance + k4 * bubbleColor * _SunIrradiance;


				vec3 _output = (1 - F) * scatter + specular + F * envReflection;
				_output = max(vec3(0), _output);
				_output = mix(_output, _FoamColor, clamp(foam,0.0,1.0));
				FragColor=vec4(_output,1);
}


