#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;

uniform sampler2D heightMap; // Contains the IFFT result
uniform float heightScale=1;   // User-defined scale for wave amplitude

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 uv;

void main() {
    uv = inTexCoord;
    // Sample the IFFT result and apply height scaling
    float height = texture(heightMap, uv).r * heightScale;
    
    // Displace the vertex along the y-axis
    vec3 displacedPos = vec3(inPosition.x, inPosition.y + height, inPosition.z);
    
    gl_Position = projection * view * model * vec4(displacedPos, 1.0);
}

