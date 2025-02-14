#version 330 core
layout (location = 0) in vec3 aPos; // Vertex position

uniform mat4 model;

void main(){

    gl_Position = model*vec4(aPos,1);

}












//void main()
//{
//    float x = aPos.x;
//    float y = aPos.y;
//    float z = aPos.z;
//
//    for (int n = 1; n <= waves; n++) {
//        float  variance = n / waves;
//        float frequency = 2.0 / max(0.1, wave_length * 2 * variance);
//        float const_speed = frequency * (speed * 2 * variance);
//        float amplitude = amplitude * 2 * variance;
//
//
//        y += amplitude * sin((aPos.y + aPos.z) * frequency + time * const_speed);
//
//    }
//
//    vec3 final_pos = vec3(x, y, z);
//    gl_Position = projection * view * model * vec4(final_pos, 1.0f);
//}