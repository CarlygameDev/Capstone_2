#version 330 core
layout (location = 0) in vec3 aPos; // Vertex position
layout (location = 1) in vec3 Normal; // Vertex position

out vec4 fragFBM;    // Pass the transformed normal to the fragment shader
out vec3 fragPosition;  // Pass the transformed position to the fragment shader


uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float time;

float amplitude = 2.0;
float wave_length = 20.0;
float speed = 25.0f;
int waves = 32;
float k = 2.5;
float warpStrength=0.1f;



void main()
{
    // Initialize in world space
    vec3 position = vec3(model*vec4(aPos,1));
    float amplitudeSum = 0.0;
    
    // Tangent vectors for normal calculation
    vec3 tangentX = vec3(1.0, 0.0, 0.0); // Initial X tangent
    vec3 tangentZ = vec3(0.0, 0.0, 1.0); // Initial Z tangent

    // Wave parameters (decay each iteration)
    float currentAmplitude = amplitude;
    float currentWavelength = wave_length;
    float h=0f;
   
    for(int i = 0; i < waves; i++) {
        // Calculate wave direction and frequency
        float phase = float(i) * 0.5;
        vec2 direction = normalize(vec2(cos(phase), sin(phase)));
        float wave_frequency = 2.0 / currentWavelength;
        float wave_speed = speed * wave_frequency + float(i)*0.1;

        // Wave equation components
        float wave_dot = dot(direction, position.xz) * wave_frequency + time * wave_speed;
        float sinWave = sin(wave_dot);
        float cosWave = cos(wave_dot);

        // Height displacement (Equation 8a)
        float waveHeight = 2.0 * currentAmplitude * pow((sinWave + 1.0)/2.0, k);
        h += waveHeight;

        // Partial derivatives (Equation 8b)
        float partialDerivativeBase = pow((sinWave + 1.0)/2.0, k - 1.0);
        float dx = k * wave_frequency * partialDerivativeBase * cosWave;


        // Domain warping (applied AFTER derivative calculation)
        vec2 warpOffset = direction*dx * warpStrength * currentAmplitude;
        tangentX.y+= warpOffset.x;
        tangentZ.y+=warpOffset.y;
        position.xz += warpOffset;

        // Prepare for next iteration
        currentAmplitude *= 0.9;    // Amplitude decay
        currentWavelength *= 1.1;   // Wavelength increase
        amplitudeSum += currentAmplitude;
    }
 
    vec3 vertexFBM=vec3(0,h,0);
    
    vec4 finalPosition=model*vec4(aPos,1);
    finalPosition.xyz+=vertexFBM;
   vec3 n=cross(tangentX,tangentZ);
    fragFBM=normalize(vec4(n,h/amplitudeSum));
    // Output transformed position
    fragPosition = finalPosition.xyz;
    gl_Position = projection * view *finalPosition;
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