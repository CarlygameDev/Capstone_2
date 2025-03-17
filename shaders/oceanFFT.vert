#version 430

layout(location = 0) in vec3 inPosition;
layout(location = 2) in vec2 inTexCoord;

uniform sampler2DArray _DisplacementTextures; 
uniform float _DisplacementDepthAttenuation=1;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 uv;
out vec3 pos;
out float depth;
float Linear01Depth(float depth) {
    float near = 0.1; // Near plane distance
    float far =10000.0f; // Far plane distance
    float z = depth * 2.0 - 1.0; // Transform from [0, 1] to [-1, 1] (NDC)
    return (2.0 * near * far) / (far + near - z * (far - near));
}
void main() {
    uv = inTexCoord;
   vec4 worldPos=model*vec4(inPosition,1);
    vec3 displacement1 = textureLod(_DisplacementTextures, vec3(worldPos.xz  , 0),0 ).rgb;
                vec3 displacement2 = textureLod(_DisplacementTextures, vec3(worldPos.xz  , 1),0).rgb ;
                vec3 displacement3 = textureLod(_DisplacementTextures,vec3(worldPos.xz  , 2),0 ).rgb;
                vec3 displacement4 = textureLod(_DisplacementTextures, vec3(worldPos.xz , 3),0).rgb;
 
    vec3 displacement = displacement1 + displacement2 + displacement3 + displacement4;
    vec4 clipPos=projection*view*model*vec4(inPosition,1);

     depth = 1 - Linear01Depth(clipPos.z / clipPos.w);
				//displacement = mix(vec3(0), displacement, pow(clamp(depth, 0.0, 1.0), _DisplacementDepthAttenuation));

                  mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 displaced_pos = inPosition.xyz + normalMatrix * displacement;
				
                worldPos= model*vec4(displaced_pos,1);
                pos=worldPos.xyz;
                gl_Position= projection*view*worldPos;
               uv = worldPos.xz;
				
}

