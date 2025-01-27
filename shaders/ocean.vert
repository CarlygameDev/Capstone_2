#version 330 core
layout (location = 0) in vec3 aPos; // Vertex position

out vec3 fragNormal;    // Pass the transformed normal to the fragment shader
out vec3 fragPosition;  // Pass the transformed position to the fragment shader

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float time;

float amplitude = 10.0;
float wave_length = 30.0;
float speed = 25.0f;
int waves = 32;
float k = 2.5;

void main()
{
    vec3 position = aPos;
    vec3 tangentX = vec3(1.0, 0.0, 0.0);
    vec3 tangentZ = vec3(0.0, 0.0, 1.0);

    float a = amplitude;
    float wl = wave_length;

    vec2 warpedXZ = position.xz; // Warped domain starts as the original domain
    float warpStrength = 0.05;    // Softer domain warping

    for (int i = 0; i < waves; i++) {
        float phase = float(i) * 0.5;
        float wave_frequency = 2.0 / wl; // Varying wavelength
        float wave_speed = speed * (2.0 / wl) + float(i) * 0.1; // Slight variation
        vec2 direction = normalize(vec2(cos(phase), sin(phase)));

        // Precompute reusable values using the warped domain
        float wave_dot = dot(warpedXZ, direction) * wave_frequency + time * wave_speed;

        // Modulate amplitude dynamically
        float adjusted_a = a * (1.0 + 0.1 * sin(time + float(i)));
        float adjusted_k = k + 0.1 * sin(time + float(i)); // Vary k slightly

        // Calculate wave displacement
        float wave_displacement = adjusted_a * pow(max((sin(wave_dot + 1.0) / 2.0), 0.0001), adjusted_k);
        position.y += wave_displacement;

        // Partial derivatives for tangents
        float wave_PD = pow(max((sin(wave_dot + 1.0) / 2.0), 0.0001), max(adjusted_k - 1.0, 0.1));
        float dx = (adjusted_k / 2.0) * direction.x * wave_frequency * adjusted_a * wave_PD * cos(wave_dot);
        float dz = (adjusted_k / 2.0) * direction.y * wave_frequency * adjusted_a * wave_PD * cos(wave_dot);

        tangentX.y += dx;
        tangentZ.y += dz;

        // Apply domain warping
        vec2 scaledDisplacement = warpStrength * vec2(dx, dz);
        warpedXZ += clamp(scaledDisplacement, vec2(-0.5), vec2(0.5));

        // Decay amplitude and increase wavelength
        a *= 0.95;
        wl *= 1.05;
    }

    tangentX = normalize(tangentX);
    tangentZ = normalize(tangentZ);

vec3 recalculatedNormal = normalize(cross(tangentZ, tangentX));
recalculatedNormal = normalize(recalculatedNormal + vec3(0.001)); // Stabilize normal
    fragPosition = position;
    fragNormal = recalculatedNormal;
    gl_Position = projection * view * model * vec4(position, 1.0f);
}

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
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