#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 ViewPos;
in float CloudHeight;

uniform float time;
uniform vec3 cloudColor = vec3(1.0, 1.0, 1.0);
uniform vec3 cloudBaseColor = vec3(0.8, 0.85, 0.9);
uniform float cloudCoverage = 0.5;
uniform float cloudSpeed = 0.01;
uniform vec3 windDirection;
uniform float cloudDensity = 1.0;
uniform samplerCube skybox;

// Improved noise function
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    // Cubic interpolation
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    // Mix 4 corners
    float a = fract(sin(dot(i, vec2(12.9898, 78.233))) * 43758.5453);
    float b = fract(sin(dot(i + vec2(1.0, 0.0), vec2(12.9898, 78.233))) * 43758.5453);
    float c = fract(sin(dot(i + vec2(0.0, 1.0), vec2(12.9898, 78.233))) * 43758.5453);
    float d = fract(sin(dot(i + vec2(1.0, 1.0), vec2(12.9898, 78.233))) * 43758.5453);
    
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// Improved FBM
float fbm(vec2 p) {
    float sum = 0.0;
    float amp = 0.5;
    float freq = 1.0;
    
    // More octaves for detail
    for(int i = 0; i < 5; i++) {
        sum += amp * noise(p * freq);
        p = vec2(p.y - p.x, p.x + p.y) * 1.1; // Rotation for more natural look
        freq *= 2.0;
        amp *= 0.5;
    }
    
    return sum;
}

// Cloud shape function with fuzzy edges
float cloudShape(vec2 uv, float time) {
    // Multiple layers with different frequencies and speeds
    float n1 = fbm(uv * 1.0 + vec2(time * 0.02, 0.0));
    float n2 = fbm(uv * 2.0 + vec2(time * 0.01, 0.0));
    float n3 = fbm(uv * 4.0 + vec2(time * 0.03, 0.0));
    
    // Combine layers
    float cloud = n1 * 0.5 + n2 * 0.3 + n3 * 0.2;
    
    // Sharpen edges based on coverage
    float threshold = 1.0 - cloudCoverage;
    cloud = smoothstep(threshold, threshold + 0.3, cloud);
    
    return cloud;
}

void main()
{
    // Calculate view direction
    vec3 viewDir = normalize(WorldPos - ViewPos);
    
    // Calculate varying noise coordinates
    vec2 baseCoord = WorldPos.xz * 0.003;
    vec2 windOffset = vec2(windDirection.x, windDirection.z) * time * cloudSpeed;
    vec2 movedCoord = baseCoord + windOffset;
    
    // Generate cloud pattern
    float cloudValue = cloudShape(movedCoord, time);
    
    // Apply texture coordinate-based softening (softer at edges)
    float distFromCenter = length((TexCoords - 0.5) * 2.0);
    
    // Different edge softening for billboards vs planes
    float edgeSoftness;
if (CloudHeight > 0.99 || CloudHeight < 0.01) {
    // For billboards, use extreme radial falloff
    edgeSoftness = 1.0 - smoothstep(0.2, 0.8, distFromCenter);
    edgeSoftness = pow(edgeSoftness, 3.0); // Much stronger falloff
} else {
    // For planes, use softer edge falloff
    edgeSoftness = 1.0 - smoothstep(0.6, 0.95, distFromCenter);
}
    
    // Apply edge softness
    cloudValue *= edgeSoftness;
    
    // Add some height-based variation
    float heightVariation = mix(0.7, 1.1, CloudHeight);
    cloudValue *= heightVariation;
    
    // Sample skybox for background blending
    vec3 skyColor = texture(skybox, viewDir).rgb;
    
    // Mix cloud color with some height-based variation
    vec3 finalCloudColor = mix(cloudBaseColor, cloudColor, CloudHeight * 0.6 + 0.4);
    
    // Add subtle shading - lighter at top
    finalCloudColor *= mix(0.9, 1.2, CloudHeight);
    
    // Calculate alpha with density control
    float alpha = cloudValue * cloudDensity * 0.8;
    
    // Blend with sky
    vec3 finalColor = mix(skyColor, finalCloudColor, alpha);
    
    // Output with proper alpha for blending
    FragColor = vec4(finalColor, alpha);
}