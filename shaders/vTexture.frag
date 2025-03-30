#version 330 core

// Input from vertex shader
in vec2 TexCoord;


// Output color
out vec4 FragColor;

// Texture sampler (sampler2DArray for a 2D texture array)
uniform sampler2DArray textureArray;
uniform sampler2D    Texture;
void main() {
vec4 height=vec4(0);
    // Sample the texture array with the given TexCoord and layerIndex
    for(int i=0;i<1;i++)
     height += texture(textureArray, vec3(TexCoord/94, 2));
    
   // height /=4;
    // Set the final fragment color
   // FragColor = vec4(height.rg,0,1);  // Convert height to grayscale (you can change this as needed)
   FragColor = vec4(height);
 //  FragColor=vec4(texture(Texture, vec2(TexCoord*0.01)));
}

