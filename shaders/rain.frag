#version 330 core
out vec4 FragColor;

in vec3 particleColor;
in float alpha;

void main()
{
    // Simple raindrop rendering with transparency
    FragColor = vec4(particleColor, alpha);
}