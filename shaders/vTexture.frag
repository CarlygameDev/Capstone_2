#version 330 core

// Input from vertex shader
in vec2 TexCoord;

// Output color
out vec4 FragColor;

// Texture sampler
uniform sampler2D texture1;

void main() {
    float height = texture(texture1, TexCoord).g;  

     

    // Convert to grayscale
  // FragColor = vec4(vec3(height), 1.0);
 FragColor =texture(texture1, TexCoord);
}
