#version 430
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D spectrumTex;   // Input: h₀(k)
layout(rgba32f, binding = 1) uniform image2D evolvedTex;     // Output: h(k, t)

uniform float time;          // Elapsed time in seconds
uniform float domainSize;    // Physical size of the ocean patch (meters)

const float PI = 3.14159265359;
const float G = 9.81;
const float RepeatTime=200;
// Complex multiplication helper
vec2 complex_mult(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 texSize = imageSize(spectrumTex);
    int N = texSize.x;
    
    // Calculate wavevector k (matches spectrum generation)
    vec4 initial_signal=imageLoad(spectrumTex, coord);
    vec2 h0= initial_signal.xy;
    vec2 h0_conj=initial_signal.zw;
  
vec2 K = (vec2(coord) - vec2(N / 2)) * (2.0 * PI / domainSize);
//    vec2 K = (coord.xy - (N/2)) * 2.0 * PI ;
  
    
   
    float kMag = length(K);
    float kMagRcp = 1/kMag;
     if (kMag < 0.0001f) {
            kMagRcp = 1.0f;
        }

        float w_0 = 2.0f * PI / RepeatTime;// /RepeatTime
        float dispersion = floor(sqrt(G*kMag)/w_0)*w_0*time;
vec2 exponent= vec2(cos(dispersion),sin(dispersion));
vec2 htilde = complex_mult(h0, exponent) + complex_mult(h0_conj, vec2(exponent.x, -exponent.y));
 vec2 ih = vec2(-htilde.y, htilde.x);

  vec2 displacementX = ih * K.x * kMagRcp;
   vec2 displacementY = htilde;
        vec2 displacementZ = ih * K.y * kMagRcp;

  vec2 displacementX_dx = -htilde * K.x * K.x * kMagRcp;
        vec2 displacementY_dx = ih * K.x;
        vec2 displacementZ_dx = -htilde * K.x * K.y * kMagRcp;

       vec2 displacementY_dz = ih * K.y;
        vec2 displacementZ_dz = -htilde * K.y * K.y * kMagRcp;

        vec2 htildeDisplacementX = vec2(displacementX.x - displacementZ.y, displacementX.y + displacementZ.x);
        vec2 htildeDisplacementZ = vec2(displacementY.x - displacementZ_dx.y, displacementY.y + displacementZ_dx.x);
        
        vec2 htildeSlopeX = vec2(displacementY_dx.x - displacementY_dz.y, displacementY_dx.y + displacementY_dz.x);
        vec2 htildeSlopeZ = vec2(displacementX_dx.x - displacementZ_dz.y, displacementX_dx.y + displacementZ_dz.x);

    imageStore(evolvedTex, coord, vec4(htildeDisplacementX,htildeDisplacementZ));
}
