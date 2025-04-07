#version 430

// Input texture holding complex values stored in the xy channels.
layout(rgba16f, binding = 0) uniform image2D PrecomputeBuffer;

const float PI = 3.1415926;
uniform int Size;
vec2 ComplexExp(vec2 a)
{
	return vec2(cos(a.y), sin(a.y)) * exp(a.x);
}

layout(local_size_x = 1, local_size_y = 8, local_size_z = 1) in;
void main(){

ivec3 id = ivec3(gl_GlobalInvocationID);
uint b = Size >> (id.x + 1);
	vec2 mult = 2 * PI * vec2(0, 1) / Size;
	uint i = (2 * b * (id.y / b) + id.y % b) % Size;
	vec2 twiddle = ComplexExp(-mult * ((id.y / b) * b));
	
imageStore (PrecomputeBuffer,id.xy,vec4(twiddle.x, twiddle.y, i, i + b))  ;
imageStore(PrecomputeBuffer,ivec2(id.x, id.y + Size / 2), vec4(-twiddle.x, -twiddle.y, i, i + b));

	
}