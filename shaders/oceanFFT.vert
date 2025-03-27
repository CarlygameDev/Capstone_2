#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;

uniform sampler2DArray _DisplacementTextures; 
uniform float _DisplacementDepthAttenuation=1;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float near = 0.1;  // Pass these from CPU
uniform float far = 10000.0;

out vec2 uv;
out vec3 pos;
out float depth;
float Linear01Depth(float ndc_z) {
    // Recover eye-space z (remember: eye-space z is negative in front of the camera)
    float z_eye = -(2.0 * near * far) / ((far + near) - ndc_z * (far - near));
    // Map eye-space z to a linear depth in [0, 1]
    // When z_eye == -near, depth should be 0; when z_eye == -far, depth should be 1.
    return (-z_eye - near) / (far - near);
}

void main() {
    uv = inTexCoord;
   vec4 worldPos=model*vec4(inPosition,1);
  uv=worldPos.xz*0.01f;
    vec3 displacement1 = textureLod(_DisplacementTextures, vec3(uv  , 0),0).rgb;
                vec3 displacement2 = textureLod(_DisplacementTextures, vec3(uv  , 1),0).rgb ;
                vec3 displacement3 = textureLod(_DisplacementTextures,vec3(uv  , 2),0 ).rgb;
                vec3 displacement4 = textureLod(_DisplacementTextures, vec3(uv , 3),0).rgb;
 
    vec3 displacement = displacement1+displacement2+displacement3+displacement4 ;
  
     vec4 clipPos = projection * view * worldPos;
    float ndc_z = clipPos.z / clipPos.w;
    depth = 1-Linear01Depth(ndc_z); 
// Calculate linear depth for fragment shader
    vec4 viewPos = view * worldPos;
 
	
    displacement = mix(vec3(0), displacement, pow(clamp(depth, 0.0, 1.0), _DisplacementDepthAttenuation));
  
                  mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 displaced_pos = inPosition.xyz + normalMatrix * displacement;
			
                worldPos= model*vec4(displaced_pos,1);
                pos=worldPos.xyz;
                gl_Position= projection*view*worldPos;
               
				
}

