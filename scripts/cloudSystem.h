#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>

// Class to manage procedural clouds
class CloudSystem {
public:
    CloudSystem(int resolution = 64);
    ~CloudSystem();

    void Render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos,
        GLuint cubemapTexture, float currentTime);

    // Cloud parameters - can be adjusted at runtime
    float cloudCoverage = 0.5f;   // 0.0 to 1.0, how much sky is covered
    float cloudDensity = 1.0f;    // Density of the clouds
    float cloudSpeed = 0.01f;     // How fast clouds move
    glm::vec3 cloudColor = glm::vec3(1.0f, 1.0f, 1.0f);  // Main cloud color
    glm::vec3 cloudBaseColor = glm::vec3(0.8f, 0.85f, 0.9f);  // Base cloud color
    glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);    // Wind direction

private:
    unsigned int VAO, VBO, EBO;
    unsigned int vertexCount;
    unsigned int indexCount;
    void InitializeCloudPlanes();
    void AddBillboardClouds(std::vector<float>& vertices, std::vector<unsigned int>& indices, float baseHeight);

};