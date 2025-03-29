#version 430

// Input texture holding complex values stored in the xy channels.
layout(rgba32f, binding = 0) uniform image2DArray Buffer0;
layout(rgba32f, binding = 1) uniform image2DArray Buffer1;
layout(rgba32f, binding = 2) uniform image2D PrecomputedData;
uniform bool PingPong;
uniform int Step;

// Helper function for complex multiplication
vec2 ComplexMult(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}


void IFFT(uint i, vec2 id)
{
	vec4 data = imageLoad(PrecomputedData,ivec2(Step, id.y));
	ivec2 inputIndices = ivec2(data.ba);
	if (PingPong)
	{
	vec4  values=imageLoad(Buffer0,ivec3(id.x, inputIndices.y,i));

	vec4 result= imageLoad(Buffer0,ivec3(id.x, inputIndices.x,i))
			+ vec4( ComplexMult(vec2(data.r, -data.g),values.rg),ComplexMult(vec2(data.r, -data.g),values.ba) );
imageStore(Buffer1,ivec3(id.xy,i), result);
	}
	else
	{
vec4  values=imageLoad(Buffer1,ivec3(id.x, inputIndices.y,i));
	vec4 result=	imageLoad(Buffer1,ivec3(id.x, inputIndices.x,i))
			+ vec4( ComplexMult(vec2(data.r, -data.g),values.rg),ComplexMult(vec2(data.r, -data.g),values.ba) );
		imageStore(Buffer0,ivec3(id.xy,i),result);
	}
}


layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;


void main() {
    ivec3 id = ivec3(gl_GlobalInvocationID.xyz);
    ivec2 coord = id.xy;

	
     for (int i = 0; i < 8; ++i) {

  
	 IFFT(i, id.xy);

  
    }
}


