
#version 430
layout(local_size_x = 16, local_size_y = 16) in;  // Workgroup size
layout(rgba16f, binding = 0) uniform image2DArray Spectrum;  // Output texture







uniform int n;
void main(){
ivec2 texSize = ivec2(n);
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
  uint i=gl_GlobalInvocationID.z;
vec2 h0=   imageLoad(Spectrum,ivec3(coord,i)).rg;

vec2 h0_conj= imageLoad(Spectrum,ivec3((texSize.x-coord.x)%texSize.x,(texSize.x-coord.y)%texSize.x,i)).rg;

imageStore(Spectrum, ivec3(coord,i), vec4(h0, h0_conj.x,-h0_conj.y));


}