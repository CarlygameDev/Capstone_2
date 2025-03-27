#version 430


#define SIZE 1024
#define LOG_SIZE 10

// Input texture holding complex values stored in the xy channels.
layout(rgba32f, binding = 0) uniform image2DArray uInput;


// Shared memory buffer for FFT computation
shared vec4 fftGroupBuffer[2][SIZE];

// Helper function for complex multiplication
vec2 complexMult(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

// Butterfly function for FFT step computation
void ButterflyValues(uint step, uint index, out uvec2 indices, out vec2 twiddle) {
    const float twoPi = 6.28318530718;
    uint b = SIZE >> (step + 1);
    uint w = b * (index / b);
    uint i = (w + index) % SIZE;

    twiddle = vec2(cos(-twoPi / SIZE * w), sin(-twoPi / SIZE * w));

    // This is what makes it the inverse FFT
    twiddle.y = -twiddle.y;

    indices = uvec2(i, i + b);
}

// FFT function
vec4 FFT(uint threadIndex, vec4 input) {
    fftGroupBuffer[0][threadIndex] = input;
    barrier();

    bool flag = false;
    for (uint step = 0; step < LOG_SIZE; step++) {
        uvec2 inputIndices;
        vec2 twiddle;
        ButterflyValues(step, threadIndex, inputIndices, twiddle);

        // Load values from the input texture at the computed indices
        vec4 v = fftGroupBuffer[uint(flag)][inputIndices.y]; // Read from the input texture

       
        fftGroupBuffer[uint(!flag)][threadIndex] = fftGroupBuffer[uint(flag)][inputIndices.x] + 
            vec4(complexMult(twiddle, v.xy), complexMult(twiddle, v.zw));

        flag = !flag;
        barrier();
    }

    return fftGroupBuffer[uint(flag)][threadIndex];
}


layout(local_size_x = SIZE, local_size_y = 1, local_size_z = 1) in;


void main() {
    ivec3 id = ivec3(gl_GlobalInvocationID.xyz);
    ivec2 coord = id.xy;
     for (int i = 0; i < 8; ++i) {
    // Load the input data from the texture
    vec4 input = imageLoad(uInput, ivec3(coord,i));

    // Perform the IFFT operation on each element
    vec4 result = FFT(gl_LocalInvocationID.x, input);

    
    // Store the result in the output texture
    imageStore(uInput, ivec3(coord,i), result);
    }
}







