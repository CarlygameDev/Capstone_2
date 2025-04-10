#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;

// Camera and fog parameters
uniform float nearPlane = 0.1;
uniform float farPlane = 5000.0;
uniform vec3 fogColor = vec3(0.7, 0.8, 0.9); // Sky-like color
uniform vec3 fogColorHorizon = vec3(0.5, 0.6, 0.8); // Horizon color (optional)
uniform float fogDensity = 0.0005;
uniform float fogStart = 300.0;
uniform float fogHeightFalloff = 0.01; // How quickly fog thins with height
uniform float fogHeight = 0.0; // World space height where fog is thickest

// Scene parameters
uniform vec3 cameraPos; // World space camera position
uniform mat4 invViewProj; // Inverse view-projection matrix

// Converts non-linear depth buffer value to linear depth
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

// Reconstruct world position from depth
vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(TexCoords * 2.0 - 1.0, z, 1.0);
    vec4 worldSpacePosition = invViewProj * clipSpacePosition;
    return worldSpacePosition.xyz / worldSpacePosition.w;
}
vec3 TonemapACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    // Fetch scene color and depth value
    vec3 sceneColor = texture(screenTexture, TexCoords).rgb;
    float depthValue = texture(depthTexture, TexCoords).r;
    
    // Linearize the depth value
    float linearDepth = LinearizeDepth(depthValue);
    
    // Reconstruct world position (for height-based fog)
    vec3 worldPos = WorldPosFromDepth(depthValue);
    
    // Calculate basic fog factor (exponential)
    float fogDistance = max(0.0, linearDepth - fogStart);
    float fogFactor = 1.0 - exp(-fogDistance * fogDensity);
    
    // Height-based fog attenuation
    float heightAboveFog = max(0.0, worldPos.y - fogHeight);
    float heightFactor = exp(-heightAboveFog * fogHeightFalloff);
    fogFactor *= heightFactor;
    
    // Optional: Horizon color blending (uncomment to enable)
    float horizonBlend = smoothstep(0.0, 1.0, 1.0 - TexCoords.y);
     vec3 finalFogColor = mix(fogColor, fogColorHorizon, horizonBlend);
 
    
    // Blend with fog color (preserve 5% of original color)
    vec3 finalColor = mix(sceneColor, finalFogColor, clamp(fogFactor, 0.0, 0.95));
    
    // Optional: Add subtle blue tint to distant objects
    float desaturation = fogFactor * 0.3;
    finalColor = mix(finalColor, finalColor * vec3(0.9, 0.95, 1.0), desaturation);
    
  //  finalColor = TonemapACES(finalColor);     
    // Apply gamma correction
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    // Output final color
    FragColor = vec4(finalColor, 1.0);
}


