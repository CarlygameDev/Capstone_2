#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>

// Class to manage rain particles
class RainSystem {
public:
    RainSystem(int particleCount = 10000);
    ~RainSystem();
    
    void Update(float deltaTime, const glm::vec3& cameraPos);
    void Render(const glm::mat4& projection, const glm::mat4& view, 
                const glm::vec3& cameraPos, float currentTime);
    
    // Rain parameters
    float rainIntensity = 0.0f;     // 0.0 = no rain, 1.0 = heavy rain
    glm::vec3 windDirection = glm::vec3(0.0f, 0.0f, 0.0f);  // Wind affecting rain
    
private:
    // Raindrop data structure
    struct RainDrop {
        glm::vec3 initialPosition;
        glm::vec3 velocity;
        float size;
        float lifeOffset;  // Random offset for particle lifetime
    };
    
    std::vector<RainDrop> raindrops;
    
    // GPU resources for rain particles
    unsigned int rainVAO, rainVBO;
    
    int maxParticles;
    
    // Use random generators for particle initialization
    std::mt19937 rng;
    std::uniform_real_distribution<float> posDistX;
    std::uniform_real_distribution<float> posDistZ;
    std::uniform_real_distribution<float> posDistY;
    std::uniform_real_distribution<float> sizeDistribution;
    std::uniform_real_distribution<float> lifeDistribution;
    
    // Methods to manage particles
    void InitializeRainParticles();
};