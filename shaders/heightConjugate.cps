
#version 430
layout(local_size_x = 16, local_size_y = 16) in;  // Workgroup size
layout(rgba32f, binding = 0) uniform image2D waveTexture;  // Output texture








void main(){
ivec2 texSize = imageSize(waveTexture);
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
vec2 h0=   imageLoad(waveTexture,coord).rg;

vec2 h0_conj= imageLoad(waveTexture,ivec2((texSize.x-coord.x)%texSize.x,(texSize.x-coord.y)%texSize.x)).rg;

imageStore(waveTexture, coord, vec4(h0, h0_conj.x,-h0_conj.y));
}