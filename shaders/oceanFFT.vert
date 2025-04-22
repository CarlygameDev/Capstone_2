#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;

uniform sampler2DArray _DisplacementTextures; 
uniform float _DisplacementDepthAttenuation=1;



void main() {          
gl_Position= vec4(inPosition,1);     
				
}

