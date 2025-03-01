#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform image2D waveTexture;  // Initial wave spectrum
layout(rgba32f, binding = 1) uniform image2D displacementTexture;  // Output time-evolved waves

uniform float time;  // Simulation time

const float PI = 3.14159265359;
const float G = 9.81;  // Gravity constant

void main()
{
    ivec2 texSize = imageSize(waveTexture);
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);

    if (coord.x >= texSize.x || coord.y >= texSize.y) return;

    vec2 uv = vec2(coord) / vec2(texSize);
    
    // Convert UV to wavevector
    float domainSize = 100.0;
    vec2 k = (uv - 0.5) * vec2(texSize) * 2.0 * PI / domainSize;
    float kLen = length(k);

    if (kLen < 1e-6)
    {
        imageStore(displacementTexture, coord, vec4(0.0));
        return;
    }

    // Get the initial spectrum h0(k)
    vec4 h0 = imageLoad(waveTexture, coord);
    vec2 h0_k = h0.rg;  // Real and imaginary parts of h0(k)
    
    // Compute dispersion relation: omega(k) = sqrt(g * k)
    float omega = sqrt(G * kLen);

    // Compute phase shift
    float phase = omega * time;
    float cosPhase = cos(phase);
    float sinPhase = sin(phase);

    // Compute h(k, t) using Euler's formula: e^(i?) = cos(?) + i sin(?)
    vec2 h_t = h0_k * vec2(cosPhase, sinPhase);  // Forward traveling wave
    vec2 h_t_conj = h0_k * vec2(cosPhase, -sinPhase);  // Backward traveling wave

    // Store the real and imaginary components of h(k, t)
    imageStore(displacementTexture, coord, vec4(h_t + h_t_conj, 0.0, 0.0));
}
