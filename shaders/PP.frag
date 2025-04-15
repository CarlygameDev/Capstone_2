#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
uniform sampler2DArray DisplacementTextures; 

// Camera and fog parameters
uniform float nearPlane = 0.1;
uniform float farPlane = 5000.0;
uniform vec3 fogColor = vec3(0.7, 0.8, 0.9);          // Sky-like color
uniform vec3 fogColorHorizon = vec3(0.5, 0.6, 0.8);     // Horizon color (optional)
uniform float fogDensity = 0.0005;
uniform float fogStart = 300.0;
uniform float fogHeightFalloff = 0.01; // How quickly fog thins with height
uniform float fogHeight = 0.0;         // World space height where fog is thickest

// Scene parameters
uniform vec3 cameraPos;              // World space camera position
uniform mat4 invViewProj;            // Inverse view-projection matrix

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

// Enhanced underwater fog that factors in view angle.
// When looking horizontally (viewDir.y near zero) the effect is stronger;
// when looking more up or down, it is softer.
vec3 computeUnderwaterFog(vec3 sceneColor, vec3 viewDir, float viewDist)
{
    // Define a gradient from shallow blue to deep blue.
    vec3 shallowBlue = vec3(0.0, 0.3, 0.4);  // Lighter blue for shallow parts
    vec3 deepBlue    = vec3(0.0, 0.1, 0.2);  // Darker blue for deeper view angles

    // The effect increases as the view direction becomes more horizontal.
    // Note: abs(viewDir.y) == 0 means horizontal view.
    float angleFactor = 1.0 - clamp(abs(viewDir.y), 0.0, 1.0);

    // Blend between shallow and deep underwater color based on the view angle.
    vec3 waterColor = mix(shallowBlue, deepBlue, angleFactor);

    // Compute a fog factor that is modulated by both distance and view angle.
    // Increase the multiplier on viewDist if you prefer a stronger depth effect.
    float fogFactor = 1.0 - exp(-viewDist * 0.05 * (0.5 + angleFactor));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    return mix(sceneColor, waterColor, fogFactor);
}

void main()
{
    // Fetch the original scene color and depth value.
    vec3 sceneColor = texture(screenTexture, TexCoords).rgb;
    float depthValue = texture(depthTexture, TexCoords).r;
    
    // Linearize the depth value.
    float linearDepth = LinearizeDepth(depthValue);
    
    // Reconstruct world position (useful for fog calculations and underwater check).
    vec3 worldPos = WorldPosFromDepth(depthValue);
    
    // Compute basic atmospheric fog for above-water view.
    float fogDistance = max(0.0, linearDepth - fogStart);
    float fogFactor = 1.0 - exp(-fogDistance * fogDensity);
    
    // Height-based fog attenuation (thicker fog closer to fogHeight).
    float heightAboveFog = max(0.0, worldPos.y - fogHeight);
    float heightFactor = exp(-heightAboveFog * fogHeightFalloff);
    fogFactor *= heightFactor;
    
    // Optional: Horizon blending for a smooth sky-to-fog transition.
    float horizonBlend = smoothstep(0.0, 1.0, 1.0 - TexCoords.y);
    vec3 finalFogColor = mix(fogColor, fogColorHorizon, horizonBlend);
    
    // Blend scene with atmospheric fog (preserve most of the scene color).
    vec3 finalColor = mix(sceneColor, finalFogColor, clamp(fogFactor, 0.0, 0.95));
    
    // Optional: Add a subtle blue tint to distant areas.
    float desaturation = fogFactor * 0.3;
    finalColor = mix(finalColor, finalColor * vec3(0.9, 0.95, 1.0), desaturation);

    // --- Underwater Check and Enhanced Effect ---
    // Sample the water displacement textures (assumed to store water-surface heights in red channel).
    float waterHeight = 0.0;
    for (int i = 0; i < 4; ++i) {
        waterHeight += texture(DisplacementTextures, vec3(TexCoords, float(i))).r;
    }
    waterHeight /= 4.0;  // Average the displacement heights

    // When the camera is below the water surface, apply the underwater effect.
    // The effect now also considers the view direction to blend appropriately.
    if (cameraPos.y < waterHeight)
    {
        vec3 viewDir = normalize(worldPos - cameraPos);
        float viewDist = length(worldPos - cameraPos);
        finalColor = computeUnderwaterFog(sceneColor, viewDir, viewDist);
    }
    
    // Apply gamma correction before output.
    finalColor = pow(finalColor, vec3(1.0 / 2.2));
    FragColor = vec4(finalColor, 1.0);
}



