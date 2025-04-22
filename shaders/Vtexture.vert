#version 330 core

// Input vertex attributes
layout(location = 0) in vec3 aPos;      // Vertex position
layout(location = 1) in vec2 aTexCoord; // Texture coordinates

// Output to fragment shader
out vec2 TexCoord;

// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



void main()
{
    // Transform the vertex position
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // Pass the texture coordinates to the fragment shader
   TexCoord = aTexCoord;
  TexCoord= vec3((model*vec4(aPos,1.0)).xyz).xz;

}