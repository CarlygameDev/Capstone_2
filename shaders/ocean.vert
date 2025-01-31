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




void main()
{
    vec3 position = vec3(model * vec4(aPos, 1.0));
    vec3 tangentX = vec3(1.0, 0.0, 0.0);
    vec3 tangentZ = vec3(0.0, 0.0, 1.0);

    float a = amplitude;
    float wl = wave_length;

   
    float warpStrength = 1;    // Softer domain warping
    	float amplitudeSum = 0.0f;
 
    for (int i = 0; i < waves; i++) {

        float phase = float(i) * 0.5;
        float wave_frequency = 2.0 / wl; // Varying wavelength
        float wave_speed = speed * (2.0 / wl) + float(i) * 0.1; // Slight variation
       
         vec2 direction = vec2(cos(phase),sin(phase));

        // Precompute reusable values using the warped domain
        float wave_dot = dot(direction, position.xz) * wave_frequency + time * wave_speed;

        
   

        // Calculate wave displacement
        float wave_displacement = a * pow(max((sin(wave_dot + 1.0) / 2.0), 0), k);
       
       position.y += wave_displacement;
    

        // Partial derivatives for tangents
        float wave_PD = pow(max((sin(wave_dot + 1.0) / 2.0), 0.0001), max(k - 1.0, 0.1));
        float dx = k  * direction.x * wave_frequency * a * wave_PD * cos(wave_dot);
        float dz = k  * direction.y * wave_frequency * a * wave_PD * cos(wave_dot);

        tangentX.y += dx;
        tangentZ.y += dz;
     
        // Apply domain warping
        vec2 scaledDisplacement = warpStrength * vec2(dx, dz);
    
        amplitudeSum+=a;
        // Decay amplitude and increase wavelength
        a *= 0.95;
        wl *= 1.05;
    }

    tangentX = normalize(tangentX);
    tangentZ = normalize(tangentZ);

vec3 recalculatedNormal = -normalize(cross(tangentX, tangentZ));
    fragPosition = position;
    fragNormal = recalculatedNormal;
    gl_Position = projection * view  * vec4(position, 1.0f);
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