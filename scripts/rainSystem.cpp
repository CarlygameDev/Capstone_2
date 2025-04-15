#include "rainSystem.h"
#include "Shader.h"
#include <iostream>
#include <ctime>

RainSystem::RainSystem(int particleCount) : maxParticles(particleCount) {
    // Initialize random number generator with seed
    rng = std::mt19937(static_cast<unsigned int>(time(nullptr)));
    
    // Setup uniform distributions for various properties
    posDistX = std::uniform_real_distribution<float>(-100.0f, 100.0f);
    posDistZ = std::uniform_real_distribution<float>(-100.0f, 100.0f);
    posDistY = std::uniform_real_distribution<float>(0.0f, 200.0f);
    sizeDistribution = std::uniform_real_distribution<float>(0.03f, 0.08f);
    lifeDistribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
    
    // Initialize particle systems
    InitializeRainParticles();
}

RainSystem::~RainSystem() {
    glDeleteVertexArrays(1, &rainVAO);
    glDeleteBuffers(1, &rainVBO);
}

void RainSystem::InitializeRainParticles() {
    // Generate particle data
    raindrops.resize(maxParticles);
    
    for (int i = 0; i < maxParticles; ++i) {
        raindrops[i].initialPosition = glm::vec3(
            posDistX(rng),
            posDistY(rng),
            posDistZ(rng)
        );
        
        // Slightly randomize velocity (mainly downwards)
        raindrops[i].velocity = glm::vec3(
            0.0f,
            -30.0f + (posDistX(rng) * 0.1f), // Some variation in speed
            0.0f
        );
        
        raindrops[i].size = sizeDistribution(rng);
        raindrops[i].lifeOffset = lifeDistribution(rng);
    }
    
    // Create GPU buffers
    glGenVertexArrays(1, &rainVAO);
    glGenBuffers(1, &rainVBO);
    
    glBindVertexArray(rainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    
    // Allocate buffer for per-instance data (position, velocity, size, life)
    glBufferData(GL_ARRAY_BUFFER, maxParticles * sizeof(RainDrop), raindrops.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RainDrop), (void*)offsetof(RainDrop, initialPosition));
    glVertexAttribDivisor(0, 1); // Instanced attribute
    
    // Velocity attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RainDrop), (void*)offsetof(RainDrop, velocity));
    glVertexAttribDivisor(1, 1);
    
    // Size attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(RainDrop), (void*)offsetof(RainDrop, size));
    glVertexAttribDivisor(2, 1);
    
    // Life offset attribute
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(RainDrop), (void*)offsetof(RainDrop, lifeOffset));
    glVertexAttribDivisor(3, 1);
    
    glBindVertexArray(0);
}

void RainSystem::Update(float deltaTime, const glm::vec3& cameraPos) {
    // Only update if rain is visible
    if (rainIntensity <= 0.01f) {
        return;
    }
    
    // For this simplified version, we don't need to update the raindrops
    // since all animation is done in the shader using the time parameter
}

void RainSystem::Render(const glm::mat4& projection, const glm::mat4& view, 
                        const glm::vec3& cameraPos, float currentTime) {
    // Skip rendering if rain intensity is too low
    if (rainIntensity <= 0.01f) {
        return;
    }
    
    static Shader rainShader("rain.vert", "rain.frag");
    
    // Set up blending for transparent particles
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth writing but keep depth testing
    glDepthMask(GL_FALSE);
    
    // Render rain drops
    rainShader.use();
    rainShader.setMat4("projection", projection);
    rainShader.setMat4("view", view);
    rainShader.setFloat("time", currentTime);
    rainShader.setVec3("cameraPos", cameraPos);
    rainShader.setFloat("rainIntensity", rainIntensity);
    rainShader.setVec3("windDirection", windDirection);
    
    glBindVertexArray(rainVAO);
    // Each raindrop needs 6 vertices for quad (2 triangles)
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, maxParticles);
    
    // Restore OpenGL state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}