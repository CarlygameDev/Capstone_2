#version 430

layout(local_size_x = 16, local_size_y = 16) in;  // Workgroup size
layout(rgba32f, binding = 0) uniform image2D waveTexture;  // Output texture

const float PI = 3.14159265359;
const float G = 9.81;  // Gravity constant

// Wind parameters (should be uniforms in production)
const vec2 windDir = normalize(vec2(.0, 1.0));
const float windSpeed = 10.0;

// JONSWAP parameters
const float alpha = 0.0081;
const float gamma = 3.3;
const float sigmaA = 0.07;
const float sigmaB = 0.09;

// Improved random number generation
float uniformRandom(vec2 uv) {
    return fract(sin(dot(uv, vec2(127.1, 311.7))) * 43758.5453);
}

float jonswapSpectrum(vec2 k) {
    float kLen = length(k);
    if (kLen < 1e-6) return 0.0;
    
    float omega = sqrt(G * kLen);
    float k_p = pow(G / (windSpeed * windSpeed), 0.333);
    float omega_p = sqrt(G * k_p);
    float sigma = omega < omega_p ? sigmaA : sigmaB;
    
    // Spectral calculations
    float r = exp(-pow(omega - omega_p, 2.0) / (2.0 * pow(sigma, 2.0)));
    float S = alpha * G*G * pow(omega, -5.0) * exp(-1.25 * pow(omega_p/omega, 4.0)) * pow(gamma, r);
    
    // Directional spreading
    vec2 kDir = normalize(k);
    float cosTheta = dot(kDir, windDir);
 // Allow waves in both directions (weaker backward)
S *= pow(abs(cosTheta), 2.0);  // Remove max()
    
    return sqrt(S);
}

void main() {
    ivec2 texSize = imageSize(waveTexture);
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    if (coord.x >= texSize.x || coord.y >= texSize.y) return;

    // Domain scaling
    const float domainSize = 512.0;
    vec2 uv = vec2(coord)/vec2(texSize);
    vec2 k = (uv - 0.5) * vec2(texSize) * 2.0 * PI / domainSize;

    // Generate random numbers with safety checks
    float rand1 = uniformRandom(uv);
    float rand2 = uniformRandom(uv + vec2(12.9898, 78.233));
    rand1 = max(rand1, 1e-8);  // Avoid log(0)
    rand2 = max(rand2, 1e-8);

    // Box-Muller transform
    float magnitude = sqrt(-2.0 * log(rand1));
    float angle = 2.0 * PI * rand2;
    vec2 h0 = sqrt(0.5 * jonswapSpectrum(k)) * vec2(
        magnitude * angle,
        magnitude * sin(angle)
    );

    imageStore(waveTexture, coord, vec4(h0, 0.0, 0.0));
}

