#version 430

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Texture bindings
layout(rgba32f, binding = 0) uniform image2DArray uInput;
layout(rgba32f, binding = 1) uniform image2DArray Displacement;
layout(rg32f, binding = 2) uniform image2DArray Slope;


// Uniform parameters
 uniform vec2 _Lambda=vec2(1,1);
uniform float _FoamDecayRate=0.0175;
uniform float _FoamBias=0.85;
uniform float _FoamThreshold;
uniform float _FoamAdd=0.1;

// Permutation function
vec4 Permute(vec4 data, uvec3 id) {
    return data * (1.0 - 2.0 * float((id.x + id.y) % 2));
}

void main() {
    for (int i = 0; i < 4; ++i) {
        uvec3 id = gl_GlobalInvocationID;
        ivec2 coord = ivec2(id.xy);
   
        vec4 htildeDisplacement = Permute(imageLoad(uInput, ivec3(coord,i*2)), id);
        vec4 htildeSlope = Permute(imageLoad(uInput,ivec3(coord,i*2+1)), id);

        vec2 dxdz = htildeDisplacement.rg;
        vec2 dydxz = htildeDisplacement.ba;
        vec2 dyxdyz = htildeSlope.rg;
        vec2 dxxdzz = htildeSlope.ba;
        
        float jacobian = (1.0f + _Lambda.x * dxxdzz.x) * (1.0f + _Lambda.y * dxxdzz.y) - _Lambda.x * _Lambda.y * dydxz.y * dydxz.y;
        vec3 displacement = vec3(_Lambda.x * dxdz.x, dydxz.x, _Lambda.y * dxdz.y);

        vec2 slopes = dyxdyz.xy / (1 + abs(dxxdzz * _Lambda));
     

        // Apply foam decay based on existing foam
        float foam = imageLoad(Displacement, ivec3(coord, i)).a;
        foam *= exp(-_FoamDecayRate);
        foam = clamp(foam, 0.0,1.0);

         float biasedJacobian = max(0.0f, -(jacobian - _FoamBias));

        if (biasedJacobian > _FoamThreshold)
            foam += _FoamAdd * biasedJacobian;
          
        imageStore(Displacement, ivec3(coord, i), vec4(displacement, foam));
        imageStore(Slope, ivec3(coord, i), vec4(slopes, 0, 0));
    }
}



