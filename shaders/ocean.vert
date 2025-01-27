#version 330 core
layout (location = 0) in vec3 aPos; // Vertex position


out vec3 fragNormal;  // Pass the transformed normal to the fragment shader
out vec3 fragPosition; // Pass the transformed position to the fragment shader

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float time;

float amplitude=20;
float wave_length=50;
float speed=25f;
int waves=64;
float k=2;
void main()
{
    vec3 position = aPos;
    vec3 tangentX = vec3(1.0, 0.0, 0.0);
    vec3 tangentZ = vec3(0.0, 0.0, 1.0);

    float a = amplitude;
    float wl = wave_length;

    vec2 warpedXZ = position.xz; // Warped domain starts as the original domain

    float warpStrength = 0.1; // Control the intensity of domain warping

    for (int i = 0; i < waves; i++) {
        float phase = float(i) * 0.5;
        float wave_frequency = 2.0 / wl; // Varying wavelength
        float wave_speed = speed * (2.0 / wl);
        vec2 direction = vec2(cos(phase), sin(phase));

        // Precompute reusable values using the warped domain
        float wave_dot = dot(warpedXZ, direction) * wave_frequency + time * wave_speed;

        // Calculate wave displacement
        float wave_displacement = a * pow((sin(wave_dot + 1.0) / 2.0), k);
        position.y += wave_displacement;

        // Partial derivatives for tangents
        float wave_PD = pow((sin(wave_dot + 1.0) / 2.0), max(k - 1.0, 1.0));
        float dx = k * direction.x * wave_frequency * a * wave_PD * cos(wave_dot);
        float dz = k * direction.y * wave_frequency * a * wave_PD * cos(wave_dot);

        tangentX.y += dx;
        tangentZ.y += dz;

        // Apply domain warping
        warpedXZ += warpStrength * vec2(dx, dz);

        // Decay amplitude and increase wavelength
        a *= 0.9;
        wl *= 1.1;
    }

    tangentX = normalize(tangentX);
    tangentZ = normalize(tangentZ);

    vec3 recalculatedNormal = normalize(cross(tangentZ, tangentX));
    fragPosition = position;
    fragNormal = recalculatedNormal;
    gl_Position = projection * view * model * vec4(position, 1.0f);
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