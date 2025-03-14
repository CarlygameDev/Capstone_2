#version 430

in vec2 uv;
out vec4 FragColor;

uniform sampler2D heightMap;  // IFFT result, assumed in the red channel
uniform vec3 lightDir;        // Should be normalized before passing to shader
uniform vec3 baseColor;

void main() {
    // Obtain texel size for finite differencing.
    vec2 texelSize = 1.0 / vec2(textureSize(heightMap, 0));

    // Sample neighboring heights with safe texture reads
    float hL = texture(heightMap, uv - vec2(texelSize.x, 0)).r;
    float hR = texture(heightMap, uv + vec2(texelSize.x, 0)).r;
    float hD = texture(heightMap, uv - vec2(0, texelSize.y)).r;
    float hU = texture(heightMap, uv + vec2(0, texelSize.y)).r;

    // Compute finite differences using central differences
    float dX = (hR - hL) / (2.0 * texelSize.x);
    float dZ = (hU - hD) / (2.0 * texelSize.y);

    // Compute the normal correctly
    vec3 normal = normalize(vec3(-dX, 1.0, -dZ));
    
    // Normalize light direction (in case it wasn't normalized before)
    vec3 lightDirNorm = normalize(lightDir);

    // Diffuse lighting calculation
    float diffuse = max(dot(normal, lightDirNorm), 0.0);
    
    // Specular lighting calculation
    vec3 viewDir = vec3(0.0, 0.0, 1.0); // Assuming the view direction is along the z-axis
    vec3 reflectDir = reflect(-lightDirNorm, normal);
    float specular = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // 32 is the shininess factor

    // Combine diffuse and specular lighting
    vec3 color = baseColor * (diffuse + specular);
    
    // Output the final color with full opacity
    FragColor = vec4(color, 1.0);
}


