
#version 430
layout(local_size_x = 16, local_size_y = 16) in;  // Workgroup size
layout(rgba32f, binding = 0) uniform image2DArray Spectrum;  // Output texture








void main(){
ivec2 texSize = imageSize(Spectrum).xy;
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    for (uint i = 0; i < 4; ++i) {
vec2 h0=   imageLoad(Spectrum,ivec3(coord,i)).rg;

vec2 h0_conj= imageLoad(Spectrum,ivec3((texSize.x-coord.x)%texSize.x,(texSize.x-coord.y)%texSize.x,i)).rg;

imageStore(Spectrum, ivec3(coord,i), vec4(h0, h0_conj.x,-h0_conj.y));
}

}