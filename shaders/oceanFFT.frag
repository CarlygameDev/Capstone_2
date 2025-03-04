#version 430

in vec2 uv;
out vec4 FragColor;

uniform sampler2D heightMap;  // IFFT result, assumed in the red channel
uniform vec3 lightDir;
uniform vec3 baseColor;

void main() {
    // Obtain texel size for finite differencing.
    vec2 texelSize = 1.0 / vec2(textureSize(heightMap, 0));

    // Sample neighboring heights
    float hL = texture(heightMap, uv - vec2(texelSize.x, 0)).r;
    float hR = texture(heightMap, uv + vec2(texelSize.x, 0)).r;
    float hD = texture(heightMap, uv - vec2(0, texelSize.y)).r;
    float hU = texture(heightMap, uv + vec2(0, texelSize.y)).r;

    // Compute finite differences using central differences
    float dX = (hR - hL) / (2.0 * texelSize.x);
    float dZ = (hU - hD) / (2.0 * texelSize.y);

    // Compute the normal: the Y component is 1 (up direction) 
    vec3 normal = normalize(vec3(-dX, 1.0, -dZ));
    
    // Diffuse lighting calculation
    float diffuse = max(dot(normal, -lightDir), 0.0);
    vec3 color = baseColor * diffuse;
    
    FragColor = vec4(color, 1.0);
}
