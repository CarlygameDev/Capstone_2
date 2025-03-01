#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform image2D inputTexture;
layout(rgba32f, binding = 1) uniform image2D outputTexture;

uniform int N;
uniform int direction;  // -1 for IFFT, 1 for FFT

shared vec4 sharedMem[16][16];

const float PI = 3.14159265359;

void bitReversePermute(inout int index, int log2N) {
    index = ((index & 0xAAAA) >> 1) | ((index & 0x5555) << 1);
    index = ((index & 0xCCCC) >> 2) | ((index & 0x3333) << 2);
    index = ((index & 0xF0F0) >> 4) | ((index & 0x0F0F) << 4);
    index = ((index & 0xFF00) >> 8) | ((index & 0x00FF) << 8);
    index >>= (16 - log2N);
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    if (coord.x >= N || coord.y >= N) return;

    int log2N = int(log2(float(N)));

    // Load data into shared memory
    vec4 value = imageLoad(inputTexture, coord);
    sharedMem[coord.x % 16][coord.y % 16] = value;
    memoryBarrierShared();
    barrier();

    // Perform FFT along X (Row-wise FFT)
    int x = coord.x;
    bitReversePermute(x, log2N);
    vec4 fftRowValue = sharedMem[x % 16][coord.y % 16];

    for (int stage = 0; stage < log2N; ++stage) {
        int halfSize = 1 << stage;
        int fullSize = halfSize * 2;

        int pairIndex = (x / fullSize) * fullSize + (x % halfSize);
        int matchIndex = pairIndex + halfSize;

        float k = float(x % fullSize) / fullSize;
        float theta = direction * 2.0 * PI * k;
        vec2 twiddle = vec2(cos(theta), sin(theta));

        vec2 f_even = sharedMem[pairIndex % 16][coord.y % 16].rg;
        vec2 f_odd  = sharedMem[matchIndex % 16][coord.y % 16].rg;

        vec2 f_odd_rotated = vec2(
            f_odd.r * twiddle.r - f_odd.g * twiddle.g,
            f_odd.r * twiddle.g + f_odd.g * twiddle.r
        );

        sharedMem[pairIndex % 16][coord.y % 16].rg = 0.5 * (f_even + f_odd_rotated);
        sharedMem[matchIndex % 16][coord.y % 16].rg = 0.5 * (f_even - f_odd_rotated);

        memoryBarrierShared();
        barrier();
    }

    // Transpose (Swap X and Y)
    vec4 transposedValue = sharedMem[coord.y % 16][coord.x % 16];
    memoryBarrierShared();
    barrier();

    // Perform FFT along Y (Column-wise FFT)
    int y = coord.y;
    bitReversePermute(y, log2N);
    vec4 fftColValue = transposedValue;

    for (int stage = 0; stage < log2N; ++stage) {
        int halfSize = 1 << stage;
        int fullSize = halfSize * 2;

        int pairIndex = (y / fullSize) * fullSize + (y % halfSize);
        int matchIndex = pairIndex + halfSize;

        float k = float(y % fullSize) / fullSize;
        float theta = direction * 2.0 * PI * k;
        vec2 twiddle = vec2(cos(theta), sin(theta));

        vec2 f_even = sharedMem[coord.x % 16][pairIndex % 16].rg;
        vec2 f_odd  = sharedMem[coord.x % 16][matchIndex % 16].rg;

        vec2 f_odd_rotated = vec2(
            f_odd.r * twiddle.r - f_odd.g * twiddle.g,
            f_odd.r * twiddle.g + f_odd.g * twiddle.r
        );

        sharedMem[coord.x % 16][pairIndex % 16].rg = 0.5 * (f_even + f_odd_rotated);
        sharedMem[coord.x % 16][matchIndex % 16].rg = 0.5 * (f_even - f_odd_rotated);

        memoryBarrierShared();
        barrier();
    }

    // Store final result
    imageStore(outputTexture, coord, sharedMem[coord.x % 16][coord.y % 16]);
}
