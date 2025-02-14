#version 450 core

in vec4 fragFBM;   
in vec3 fragPosition; 

out vec4 FragColor; 

// Corrected cubemap uniform type
uniform samplerCube skybox;  
uniform vec3 cameraPos;
uniform vec3 sunDirection; 
uniform vec3 sunColor;  
uniform mat3 normalMatrix;

vec3 tipColor = vec3(0.7f, 0.8f, 0.9f);
float tipStrength = 0.5;

void main()
{
    vec3 lightDir = normalize(-sunDirection);  
    vec3 lightColor = sunColor;  

    float height = fragFBM.w;
    vec3 normal = normalize(vec3(fragFBM.x, -fragFBM.y, fragFBM.z));

    vec3 oceanColor = vec3(0.0, 0.3, 0.7);
    vec3 viewDir = normalize(cameraPos - fragPosition);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // Ambient Lighting (Sun Color Affects It)
    vec3 ambient = 0.2 * oceanColor * lightColor;
    
    // Diffuse Lighting (Sun Color Applied)
    float ndotl = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = lightColor * ndotl * oceanColor;

    // Specular Reflection (Boosted Sun Glint)
    float roughness = 0.05;  // Reduced for sharper reflections
    float shininess = 2.0 / (roughness * roughness) - 2.0;
    float specularTerm = pow(max(dot(normal, halfwayDir), 0.0), shininess) * (shininess + 8.0) / (8.0 * 3.14159);
    vec3 specular = specularTerm * lightColor * 1.2;  // **Increased intensity for sun reflection**

    // Fresnel Reflection (Adjusted for Proper Blending)
    float F0 = 0.02;  
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);
    fresnel = mix(F0, 1.0, fresnel);

    // Cube Map Reflection
    vec3 reflectionVec = reflect(-viewDir, normal);
    vec3 reflectionColor = texture(skybox, reflectionVec).rgb;

    // **Balanced Fresnel Reflection to Preserve Specular**
  //  vec3 fresnelReflection = mix(specular , reflectionColor, fresnel);
   vec3 fresnelReflection = mix(reflectionColor , specular, fresnel*height);
    // Foam Effect
    float foamThreshold = 0.8;
    float foam = smoothstep(foamThreshold - 0.2, foamThreshold + 0.2, height) * tipStrength;
    vec3 foamColor = mix(tipColor, oceanColor, foam);

    // Final Color Composition (Better Blending)
    vec3 finalColor = ambient + diffuse+specular+fresnelReflection+ foam;
    finalColor = mix(finalColor, oceanColor, 0.5); // Less aggressive blending for depth

    FragColor = vec4(finalColor, 1.0);
}




