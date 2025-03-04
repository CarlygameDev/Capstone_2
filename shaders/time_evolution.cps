#version 430
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D spectrumTex;   // Input: h₀(k)
layout(rgba32f, binding = 1) uniform image2D evolvedTex;     // Output: h(k, t)

uniform float time;          // Elapsed time in seconds
uniform float domainSize;    // Physical size of the ocean patch (meters)

const float PI = 3.14159265359;
const float G = 9.81;

// Complex multiplication helper
vec2 complex_mult(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 texSize = imageSize(spectrumTex);
    int N = texSize.x;
    
    // Calculate wavevector k (matches spectrum generation)
    vec2 uv = vec2(coord) / vec2(texSize);
    vec2 k = (uv - 0.5) * 2.0 * PI / (domainSize / texSize.x);
    
    // Fetch initial spectrum value h₀(k)
    vec4 h0 = imageLoad(spectrumTex, coord);
    
    // Calculate angular frequency ω = sqrt(g * |k|)
    float kLen = length(k);
    float omega = sqrt(G * kLen);
    
    // Calculate mirrored coordinate for -k
    ivec2 mirroredCoord = ivec2(N - 1 - coord.x, N - 1 - coord.y);
    vec4 h0_mirrored = imageLoad(spectrumTex, mirroredCoord);
    
    // Time evolution factors
    vec2 exp_pos = vec2(cos(omega * time), sin(omega * time));  // e^(iωt)
    vec2 exp_neg = vec2(exp_pos.x, -exp_pos.y);                 // e^(-iωt)
    
    // Compute h(k, t) = h₀(k)e^(iωt) + h₀*(-k)e^(-iωt)
    vec2 term1 = complex_mult(h0.xy, exp_pos);
    vec2 term2 = complex_mult(vec2(h0_mirrored.x, -h0_mirrored.y), exp_neg);
    vec2 h_t = term1 + term2;
    
    imageStore(evolvedTex, coord, vec4(h_t, 0.0, 0.0));
}
