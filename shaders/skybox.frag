#version 330 core
out vec4 FragColor;

in vec3 TexCoords; // This is the direction for the current skybox fragment
uniform samplerCube skybox;
uniform vec3 sunDirection; // Should be normalized already
uniform vec3 sunColor;
uniform float sunSize=50; // The cosine of the maximum angle for the sun's disk

void main()
{
    // Fetch the base sky color from the cubemap.
    vec4 skyColor = texture(skybox, TexCoords);
    
    // Normalize the fragment's direction.
    vec3 fragDir = normalize(TexCoords);
    
    // Ensure sunDirection is normalized (if not already).
    vec3 sunDir = -normalize(sunDirection);
    
    // Calculate how closely aligned this fragment is with the sun.
    float alignment = dot(fragDir, sunDir);
    
    // Option 1: Use a sharp cutoff with a power function.
    // This will give a very sharp sun disk.
    float intensity = pow(clamp(alignment, 0.0, 1.0), 3500.0);
    
    // Option 2 (Alternative): Use smoothstep for a softer edge.
    // float intensity = smoothstep(sunSize, sunSize + 0.01, alignment);
    
    // Combine the sun color scaled by the intensity.
    vec3 sunContribution = sunColor * intensity;
    
    // Add the sun contribution to the sky color.
    vec3 finalColor = skyColor.rgb + sunContribution;
    
    FragColor = vec4(finalColor, skyColor.a);
}
