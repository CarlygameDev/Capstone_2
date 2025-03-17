#version 330 core

// Input from vertex shader
in vec2 TexCoord;


// Output color
out vec4 FragColor;

// Texture sampler (sampler2DArray for a 2D texture array)
uniform sampler2DArray textureArray;

void main() {
vec4 height=vec4(0);
    // Sample the texture array with the given TexCoord and layerIndex
    for(int i=0;i<1;i++)
     height += texture(textureArray, vec3(TexCoord, i));
    
   // height /=4;
    // Set the final fragment color
    FragColor = vec4(height.aaa,1);  // Convert height to grayscale (you can change this as needed)
}

