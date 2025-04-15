#version 330 core
layout (location = 0) in vec3 aPos;      // Initial position
layout (location = 1) in vec3 aVelocity; // Initial velocity
layout (location = 2) in float aSize;    // Droplet size
layout (location = 3) in float aLife;    // Lifetime offset

out vec3 particleColor;
out float alpha;

uniform mat4 projection;
uniform mat4 view;
uniform float time;
uniform vec3 cameraPos;
uniform float rainIntensity;  // Controls how much rain is visible
uniform vec3 windDirection;   // Direction of wind affecting rain

void main()
{
    // Rain physics parameters
    float gravity = -9.8;
    float maxLifetime = 5.0;
    float initLifetime = aLife * maxLifetime;
    
    // Calculate current lifetime of particle
    float lifetime = mod(time + initLifetime, maxLifetime);
    
    // Calculate position with physics
    vec3 pos = aPos;
    pos.y += aVelocity.y * lifetime + 0.5 * gravity * lifetime * lifetime;
    
    // Apply wind to position
    pos.x += windDirection.x * lifetime * 2.0;
    pos.z += windDirection.z * lifetime * 2.0;
    
    // Make rain fall from around the camera
    pos.x += cameraPos.x;
    pos.z += cameraPos.z;
    
    // Calculate alpha based on lifetime (fade in/out)
    alpha = 1.0;
    if (lifetime < 0.1) alpha = lifetime / 0.1;
    if (lifetime > maxLifetime - 0.1) alpha = (maxLifetime - lifetime) / 0.1;
    
    // Scale alpha by rain intensity
    alpha *= rainIntensity;
    
    // Color calculation (blue-white tint)
    particleColor = vec3(0.7, 0.7, 0.85);
    
    // Billboard effect - always face camera
    float size = aSize * 0.5;
    
    // Create quad vertices for billboard
    vec3 cameraRight = normalize(vec3(view[0][0], view[1][0], view[2][0]));
    vec3 cameraUp = normalize(vec3(view[0][1], view[1][1], view[2][1]));
    
    // Convert rain drops to elongated lines for more realistic effect
    // Stretch droplets in direction of velocity and wind
    vec3 stretchDir = normalize(vec3(windDirection.x, gravity, windDirection.z));
    float stretchFactor = 4.0; // Elongate 4x in fall direction
    
    // Select which corner of the billboard quad this is
    int vertexID = gl_VertexID % 6;
    vec3 vertexPos = pos;
    
    if (vertexID == 0) vertexPos += (-cameraRight + stretchDir * stretchFactor) * size;
    else if (vertexID == 1) vertexPos += (-cameraRight - stretchDir * stretchFactor) * size;
    else if (vertexID == 2) vertexPos += (cameraRight - stretchDir * stretchFactor) * size;
    else if (vertexID == 3) vertexPos += (cameraRight - stretchDir * stretchFactor) * size;
    else if (vertexID == 4) vertexPos += (cameraRight + stretchDir * stretchFactor) * size;
    else vertexPos += (-cameraRight + stretchDir * stretchFactor) * size;
    
    gl_Position = projection * view * vec4(vertexPos, 1.0);
}