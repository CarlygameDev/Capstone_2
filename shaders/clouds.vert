#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in float aBillboard;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 ViewPos;
out float CloudHeight;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 cameraPos;

void main()
{
    TexCoords = aTexCoords;
    ViewPos = cameraPos;
    
    if (aBillboard > 0.5) {
        // Billboard mode - always face camera
        vec3 pos = vec3(model * vec4(aPos, 1.0)); // Apply model transform to position
        
        // Calculate billboard vectors
        vec3 cameraRight = normalize(vec3(view[0][0], view[1][0], view[2][0]));
        vec3 cameraUp = normalize(vec3(view[0][1], view[1][1], view[2][1]));
        
        // Calculate offset direction (from center to corner)
        vec2 offset = aTexCoords * 2.0 - 1.0; // Convert 0-1 to -1 to 1
        
        // Create the billboard quad
        float size = length(aPos - vec3(aPos.x, aPos.y, aPos.z)); // Extract size from position
        WorldPos = pos + (cameraRight * offset.x + cameraUp * offset.y) * size;
        
        // Final position
        gl_Position = projection * view * vec4(WorldPos, 1.0);
        
        // Set cloud height (use 1.0 to identify billboards in fragment shader)
        CloudHeight = 1.0;
    } else {
        // Regular plane
        WorldPos = vec3(model * vec4(aPos, 1.0));
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        
        // Calculate normalized height for regular clouds
        CloudHeight = (aPos.y - 150.0) / 60.0; // Normalize to 0-1 range
    }
}