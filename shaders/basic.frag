#version 450 core

in vec4 fragFBM;   
in vec3 fragPosition; 

out vec4 FragColor; 


uniform vec3 cameraPos;
uniform vec3 sunDirection; 
uniform vec3 sunColor;
uniform mat3 normalMatrix;
vec3 tipColor= vec3(0.7f,0.8f,0.9f);
float tipStrength=0.3;
vec3 _DiffuseReflectance = vec3(0.0, 0.5, 1.0);

float _SpecularStrength=1;



void main()
{
    vec3 lightDir = normalize(-vec3(-0.2f, -1.0f, -0.9f));  
    vec3 lightColor = vec3(1); 

    float height=0;
    vec3 normal=vec3(0);
    vec3 oceanColor = vec3(0.0, 0.3, 0.7); // Slightly darker to compensate
   // float shininess = 64.0; // Lowered for softer specular
    height = fragFBM.w;
	normal = fragFBM.xyz;
normal= vec3(normal.x,-normal.y,normal.z);
    // Normalize inputs
     normal = normalize(normal);
    vec3 viewDir = normalize(cameraPos - fragPosition);
        vec3 halfwayDir = normalize(lightDir + viewDir);
    // Ambient lighting
    
    
    
  vec3 ambient = 0.1 * oceanColor;
    float ndotl = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = lightColor * ndotl * oceanColor;

    // Specular (Cook-Torrance)

    float roughness = 0.05;
    float shininess = 2.0 / (roughness * roughness) - 2.0;
    float specularTerm = pow(max(dot(normal, halfwayDir), 0.0), shininess) * (shininess + 8.0) / (8.0 * 3.14159);
    vec3 specular = specularTerm * lightColor * 0.5;
   
   float F0 = 0.02;  
  float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);
fresnel = mix(F0, 1.0, fresnel);
vec3 fresnelReflection = fresnel * lightColor * 2.0; // Increase intensity


 float foamThreshold = 1.2;
float foam = smoothstep(foamThreshold - 0.1, foamThreshold + 0.1, height);
vec3 foamColor = mix(tipColor, oceanColor, foam);
    // Combine components
    vec3 finalColor = ambient + diffuse + specular + fresnelReflection+foam;
    finalColor = mix(finalColor, oceanColor, 0.5); // Blend with base color
      
    FragColor = vec4(finalColor, 1.0);
}

