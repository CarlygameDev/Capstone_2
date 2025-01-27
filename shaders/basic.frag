#version 330 core

in vec3 fragNormal;   // Transformed normal from the vertex shader
in vec3 fragPosition; // Transformed position from the vertex shader

out vec4 FragColor; // Final fragment color
uniform vec3 cameraPos;
void main()
{
    // Hardcoded values for lighting and ocean properties
    vec3 lightDir = normalize(-vec3(-0.2f, -1.0f, -0.3f)); 
 
          
 vec3 oceanColor = vec3(0.0, 0.4, 0.8);             // Deep blue ocean color
 //  vec3 oceanColor = vec3(1.0, 1.0, 1.0);             // Deep blue ocean color
    float shininess = 32.0;                            // Shininess for specular highlights

    // Normalize inputs
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(cameraPos - fragPosition);

    // Ambient lighting
    vec3 ambient = 0.1 * oceanColor;

    // Diffuse lighting (Lambertian reflection)
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff  * oceanColor;

    // Specular lighting (Blinn-Phong reflection model)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
   
   vec3 lightColor = vec3(0.5); // Add this at the start of the shader
vec3 specular = spec * lightColor;
    // Combine lighting components
 vec3 finalColor = ambient + diffuse + specular;

    // Output final color
    FragColor = vec4(finalColor, 1.0);
}
