#version 330 core
out vec4 FragColor;

in vec3 TexCoords; 
uniform samplerCube skybox;
uniform samplerCube mountainMask;
uniform samplerCube starbox;
uniform vec3 sunDirection; 
uniform vec3 sunColor;
uniform float sunSize=0.9995;
uniform float timeOfDay; // normalized: 0.0 = midnight, 0.5 = noon, 1.0 = next midnight 


void main() {

    // base sky computation 
    vec4 skyColor = texture(skybox, TexCoords);
    float dayFactor = clamp(sin(timeOfDay * 3.14159), 0.0, 1.0);
    vec3 blendedSky = mix(skyColor.rgb * 0.3, skyColor.rgb, dayFactor);
    
    //  gradient based on fragment direction
    float verticalFactor = (TexCoords.y + 1.0) * 0.5;
    vec3 gradientBottom;
    vec3 gradientTop;

    if (timeOfDay < 0.15) {
        // Night
        gradientBottom = vec3(0.267, 0.251, 0.482);
        gradientTop    = vec3(0.082, 0.106, 0.23);
    } else if (timeOfDay < 0.3) {
        // early sunrise
        float factor = (timeOfDay - 0.15) / 0.15;
        gradientBottom = mix(vec3(0.267, 0.251, 0.482), vec3(0.984, 0.345, 0.424), factor);
        gradientTop    = mix(vec3(0.082, 0.106, 0.23), vec3(0.212, 0.275, 0.553), factor);
    } else if (timeOfDay < 0.4) {
        // late sunrise
        float factor = (timeOfDay - 0.3) / 0.1;
        gradientBottom = mix(vec3(0.984, 0.345, 0.424), vec3(0.996, 0.655, 0.345), factor);
        gradientTop    = mix(vec3(0.212, 0.275, 0.553), vec3(0.588, 0.702, 0.831), factor);
    } else if (timeOfDay < 0.6) {
        // midday
        float factor = (timeOfDay - 0.4) / 0.2;
        gradientBottom = mix(vec3(0.996, 0.655, 0.345), vec3(0.678, 0.816, 0.941), factor);
        gradientTop    = mix(vec3(0.588, 0.702, 0.831), vec3(0.337, 0.592, 0.831), factor);
    } else if (timeOfDay < 0.7) {
        // early sunset
        float factor = (timeOfDay - 0.6) / 0.1;
        gradientBottom = mix(vec3(0.678, 0.816, 0.941), vec3(0.996, 0.655, 0.345), factor);
        gradientTop    = mix(vec3(0.337, 0.592, 0.831), vec3(0.588, 0.702, 0.831), factor);
    } else if (timeOfDay < 0.85) {
        // late sunrset
        float factor = (timeOfDay - 0.7) / 0.15;
        gradientBottom = mix(vec3(0.996, 0.655, 0.345), vec3(0.984, 0.345, 0.424), factor);
        gradientTop    = mix(vec3(0.588, 0.702, 0.831), vec3(0.212, 0.275, 0.553), factor);
    } else {
        // night
        float factor = (timeOfDay - 0.85) / 0.15;
        gradientBottom = mix(vec3(0.984, 0.345, 0.424), vec3(0.267, 0.251, 0.482), factor);
        gradientTop    = mix(vec3(0.212, 0.275, 0.553), vec3(0.082, 0.106, 0.23), factor);
    }

    vec3 verticalGradient = mix(gradientBottom, gradientTop, verticalFactor);
    vec3 finalSky = mix(blendedSky, verticalGradient, 0.7);

    // sun calculation 
    vec3 fragDir = -normalize(TexCoords);
    vec3 sunDir = normalize(sunDirection);
    float alignment = dot(fragDir, sunDir); // Calculate how closely aligned this fragment is with the sun
    float intensity = smoothstep(sunSize, sunSize + 0.05, alignment); // softens sun edges
    vec3 sunContribution = sunColor * intensity * 5.0; 
    
    // mountain mask for occlusion
    float maskValue = texture(mountainMask, TexCoords).r;
    float maskFactor = 1.0 - maskValue;
    sunContribution *= maskFactor;

    // add the sun contribution to the sky color
    vec3 finalColor = mix(finalSky, sunColor, intensity);

    // allow some of the sun inensity to pass through the mountain mask
    float sunriseEffect = smoothstep(0.15, 0.4, timeOfDay);
    float sunsetEffect  = smoothstep(0.75, 0.6, timeOfDay);
    float timeEffect = max(sunriseEffect, sunsetEffect);
    float mountainGlowFactor = maskValue; 
    vec3 sunGlow = sunColor * timeEffect * mountainGlowFactor * intensity * 0.3;

    // blend stars 
    vec3 starColor = texture(starbox, TexCoords).rgb;
    starColor *= 0.3;
    starColor *= maskFactor;
    // compute based on the night time window
    float fadeIn  = smoothstep(0.90, 1.0, timeOfDay);
    float fadeOut = smoothstep(0.10, 0.0, timeOfDay);
    float starAlpha = max(fadeIn, fadeOut);
    
    vec3 skyWithStars = mix(finalSky, starColor, starAlpha * 0.6);

    // FragColor = vec4(skyWithStars + sunContribution, 1.0);
    FragColor = vec4(skyWithStars + sunContribution + sunGlow, 1.0);
}
