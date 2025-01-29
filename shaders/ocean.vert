#version 330 core
layout (location = 0) in vec3 aPos; // Vertex position

out vec3 fragNormal;    // Pass the transformed normal to the fragment shader
out vec3 fragPosition;  // Pass the transformed position to the fragment shader

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float time;

float amplitude = 10.0;
float wave_length = 40.0;
float speed = 25.0f;
int waves = 32;
float k = 2;

// Wind parameters
uniform vec2 windDirection = vec2(1.0, 0.0); // Normalized wind direction
uniform float windSpread = 0.3;              // Directional variance (0-1)

// Random number generator with seed
float hash(float n) {
    return fract(sin(n)*43758.5453);
}

vec2 poissonDiskDir(int i) {
    // Poisson disk-style directions with golden angle spiral
    const float GOLDEN_ANGLE = 2.39996323; // ~137.5 degrees in radians
    float theta = i * GOLDEN_ANGLE;
    float r = sqrt(float(i) + 0.5) / sqrt(float(waves));
    return vec2(cos(theta), sin(theta)) * r;
}

vec2 calculateWaveDirection(int index) {
    // Base direction from wind influence
    vec2 baseDir = normalize(windDirection);
    
    // Generate Poisson disk distribution
    vec2 poissonDir = poissonDiskDir(index);
    
    // Random perturbation
    float randAngle = hash(float(index)*12.9898) * 6.283185 * windSpread;
    vec2 randOffset = vec2(cos(randAngle), sin(randAngle)) * 0.15;
    
    // Combine directions
    vec2 finalDir = normalize(baseDir + poissonDir + randOffset);
    
    return finalDir;
}
void main()
{
    vec3 position = vec3(model * vec4(aPos, 1.0));
    vec3 tangentX = vec3(1.0, 0.0, 0.0);
    vec3 tangentZ = vec3(0.0, 0.0, 1.0);

    float a = amplitude;
    float wl = wave_length;

    vec2 warpedXZ = position.xz; // Warped domain starts as the original domain
    float warpStrength = 1;    // Softer domain warping

    for (int i = 0; i < waves; i++) {

        float phase = float(i) * 0.5;
        float wave_frequency = 2.0 / wl; // Varying wavelength
        float wave_speed = speed * (2.0 / wl) + float(i) * 0.1; // Slight variation
       
         vec2 direction = calculateWaveDirection(i);

        // Precompute reusable values using the warped domain
        float wave_dot = dot(warpedXZ, direction) * wave_frequency + time * wave_speed;

        // Modulate amplitude dynamically
        float adjusted_a = a * (1.0 + 0.1 * sin(time + float(i)));
        float adjusted_k = k + 0.1 * sin(time + float(i)); // Vary k slightly

        // Calculate wave displacement
        float wave_displacement = adjusted_a * pow(max((sin(wave_dot + 1.0) / 2.0), 0), adjusted_k);
        position.y += wave_displacement;

        // Partial derivatives for tangents
        float wave_PD = pow(max((sin(wave_dot + 1.0) / 2.0), 0.0001), max(adjusted_k - 1.0, 0.1));
        float dx = (adjusted_k / 2.0) * direction.x * wave_frequency * adjusted_a * wave_PD * cos(wave_dot);
        float dz = (adjusted_k / 2.0) * direction.y * wave_frequency * adjusted_a * wave_PD * cos(wave_dot);

        tangentX.y += dx;
        tangentZ.y += dz;

        // Apply domain warping
        vec2 scaledDisplacement = warpStrength * vec2(dx, dz);
        warpedXZ += scaledDisplacement;

        // Decay amplitude and increase wavelength
        a *= 0.95;
        wl *= 1.05;
    }

    tangentX = normalize(tangentX);
    tangentZ = normalize(tangentZ);
    position=vec3(warpedXZ.x,position.y,warpedXZ.y);
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