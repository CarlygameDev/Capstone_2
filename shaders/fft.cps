#version 430

// Workgroup size; adjust as needed.
layout (local_size_x = 16, local_size_y = 16) in;

// Input texture holding complex values stored in the xy channels.
layout (rgba32f, binding = 0) uniform image2D uInput;
// Output texture for writing the FFT pass results.
layout (rgba32f, binding = 1) uniform image2D uOutput;

// Uniforms to control the FFT pass.
uniform int uStage;      // Current FFT stage (0 to log2(N)-1)
uniform int uDirection;  // 0 = horizontal pass, 1 = vertical pass
uniform int uSize;       // The FFT size (assumed square and power-of-two)

const float PI = 3.14159265359;

void main() {
    ivec2 gid = ivec2(gl_GlobalInvocationID.xy);

    // Select the processing axis based on the pass direction.
    // For horizontal pass (uDirection==0), index is x; for vertical, it's y.
    int index = (uDirection == 0) ? gid.x : gid.y;
    // The constant coordinate along the other axis.
    int constIndex = (uDirection == 0) ? gid.y : gid.x;

    // Determine the butterfly group parameters.
    int halfSize = 1 << uStage;      // 2^stage
    int fullSize = halfSize << 1;      // 2^(stage+1)
    
    // Determine our position within the current butterfly group.
    int butterflyIndex = index % fullSize;
    // Base index of the butterfly group.
    int baseIndex = index - butterflyIndex;
    
    // The two indices to be combined.
    int indexA = baseIndex + (butterflyIndex % halfSize);
    int indexB = baseIndex + (butterflyIndex % halfSize) + halfSize;
    
    // Compute coordinates in the texture.
    ivec2 coordA = (uDirection == 0) ? ivec2(indexA, constIndex)
                                     : ivec2(constIndex, indexA);
    ivec2 coordB = (uDirection == 0) ? ivec2(indexB, constIndex)
                                     : ivec2(constIndex, indexB);
    
    // Load the two complex numbers from the input texture.
    // We store complex numbers in the .xy components.
    vec2 a = imageLoad(uInput, coordA).xy;
    vec2 b = imageLoad(uInput, coordB).xy;
    
    // Calculate the twiddle factor.
    // For a forward FFT, the exponent sign is negative.
 
 // Change the angle sign for inverse FFT:
float angle = +2.0 * PI * float(butterflyIndex % halfSize) / float(fullSize);
    vec2 twiddle = vec2(cos(angle), sin(angle));
    
    // Multiply b by the twiddle factor (complex multiplication).
    vec2 bTwiddled = vec2(
        b.x * twiddle.x - b.y * twiddle.y,
        b.x * twiddle.y + b.y * twiddle.x
    );
    
    // Butterfly computations.
    vec2 outA = a + bTwiddled;
    vec2 outB = a - bTwiddled;
    
    // Write the computed results back to the output texture.
    // Which coordinate gets updated depends on the butterfly index.
    if (butterflyIndex < halfSize)
        imageStore(uOutput, coordA, vec4(outA, 0.0, 0.0));
    else
        imageStore(uOutput, coordB, vec4(outB, 0.0, 0.0));
}

