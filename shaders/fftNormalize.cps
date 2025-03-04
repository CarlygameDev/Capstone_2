#version 430
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D uInput;
uniform float normFactor;  // 1.0 / (N * N)

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    vec4 pixel = imageLoad(uInput, coord);
    pixel *= normFactor;
    imageStore(uInput, coord, pixel);
}

