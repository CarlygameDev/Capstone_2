#version 430

layout(local_size_x = 16, local_size_y = 16) in;  // Workgroup size

layout(rgba32f, binding = 0) uniform image2D waveTexture;  // Output texture

const float PI = 3.14159265359;
const float G = 9.81;  // Gravity constant

// Wind parameters
const vec2 windDir = normalize(vec2(1.0, 1.0));  // Wind direction
const float windSpeed = 10.0;  // Wind speed in m/s

// JONSWAP parameters
const float alpha = 0.0081;
const float gamma = 3.3;
const float sigmaA = 0.07;
const float sigmaB = 0.09;

// Function to generate Gaussian random numbers (high quality)
float gaussianRandom(vec2 uv)
{
    float rand1 = fract(sin(dot(uv, vec2(127.1, 311.7))) * 43758.5453123);
    float rand2 = fract(sin(dot(uv + vec2(1.0, 1.0), vec2(269.5, 183.3))) * 43758.5453123);
    return rand1;
}

// Compute JONSWAP spectrum
float jonswapSpectrum(float k)
{
    float omega = sqrt(G * k);
    float k_p = pow(G / (windSpeed * windSpeed), 0.333);  // Peak wavenumber
    float omega_p = sqrt(G * k_p);  // Peak frequency
    float sigma = omega < omega_p ? sigmaA : sigmaB;
    float r = exp(-pow((omega - omega_p), 2.0) / (2.0 * sigma * sigma));  // JONSWAP shape factor

    float S = alpha * G * G * pow(omega, -5.0) * exp(-1.25 * pow(omega_p / omega, 4.0)) * pow(gamma, r);
    return sqrt(S);  // Convert spectrum to amplitude
}

void main()
{
    ivec2 texSize = imageSize(waveTexture);
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);

    // Bounds check
    if (coord.x >= texSize.x || coord.y >= texSize.y) return;

    vec2 uv = vec2(coord) / vec2(texSize);

    // Convert UV to wavevector (scaled by domain size)
    float domainSize = 512.0;  // You can adjust the domain size
    vec2 k = (uv - 0.5) * vec2(texSize) * 2.0 * PI / domainSize;  // Scale wave vectors
    float kLen = length(k);

    if (kLen < 1e-6)
    {
        imageStore(waveTexture, coord, vec4(0.0));  // Avoid division by zero
        return;
    }

    // Compute JONSWAP spectrum
    float spectrum = jonswapSpectrum(kLen);

    // Generate Gaussian random numbers
    float rand1 = gaussianRandom(uv);
    float rand2 = gaussianRandom(uv + 0.1);

    float xi_r = sqrt(-2.0 * log(rand1)) * cos(2.0 * PI * rand2);
    float xi_i = sqrt(-2.0 * log(rand1)) * sin(2.0 * PI * rand2);

    // Compute h0(k) = sqrt(S(k)/2) * (ξr + i ξi)
    vec2 h0 = sqrt(0.5 * spectrum) * vec2(xi_r, xi_i);

    // Store in texture (real and imaginary parts)
    imageStore(waveTexture, coord, vec4(h0, 0.0, 0.0));
}

