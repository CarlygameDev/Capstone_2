#version 430

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Texture bindings
layout (rgba32f, binding = 0) uniform image2D uInput;




// Uniform parameters
 vec2 uLambda=vec2(1,1);
uniform float uFoamDecayRate;
uniform float uFoamBias;
uniform float uFoamThreshold;
uniform float uFoamAdd;

// Permutation function
vec4 Permute(vec4 data, uvec3 id) {
    return data * (1.0 - 2.0 * float((id.x + id.y) % 2));
}

void main() {
    uvec3 id = gl_GlobalInvocationID;
    ivec2 coord = ivec2(id.xy);

   
        vec4 htildeDisplacement = Permute(imageLoad(uInput, ivec2(coord)), id);

        vec2 dxdz = htildeDisplacement.rg;
        vec2 dydxz = htildeDisplacement.ba;
        
        

        vec3 displacement = vec3(uLambda.x * dxdz.x, dydxz.x, uLambda.y * dxdz.y);

       
     



      


        imageStore(uInput, coord, vec4(displacement, 1));
  
}


