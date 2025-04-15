#include "cloudSystem.h"
#include "Shader.h"
#include <iostream>

// CloudSystem Implementation
CloudSystem::CloudSystem(int resolution) {
    InitializeCloudPlanes();
}

CloudSystem::~CloudSystem() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void CloudSystem::InitializeCloudPlanes() {
    // Create multiple cloud planes at different heights and scales
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Setup cloud planes at different heights
    const int numPlanes = 5;  // Keep it at 5 planes for now
    const float baseHeight = 180.0f;  // Higher base height to avoid ocean clipping
    const float planeSize = 800.0f;  // Smaller size for less rectangular appearance

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> offsetDist(-100.0f, 100.0f);
    std::uniform_real_distribution<float> scaleDist(0.7f, 1.3f);

    for (int layer = 0; layer < numPlanes; ++layer) {
        // Vary heights slightly between layers
        float y = baseHeight + layer * 25.0f + offsetDist(gen) * 0.2f;
        float scale = scaleDist(gen);
        float currentSize = planeSize * scale;

        // Add variation to each corner to break up the rectangular shape
        float offset1 = offsetDist(gen) * 0.3f;
        float offset2 = offsetDist(gen) * 0.3f;
        float offset3 = offsetDist(gen) * 0.3f;
        float offset4 = offsetDist(gen) * 0.3f;

        // Bottom left - with some variation
        vertices.push_back(-currentSize + offset1);
        vertices.push_back(y);
        vertices.push_back(-currentSize + offset2);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);  // Not a billboard

        // Bottom right - with variation
        vertices.push_back(currentSize + offset2);
        vertices.push_back(y);
        vertices.push_back(-currentSize + offset3);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);

        // Top right - with variation
        vertices.push_back(currentSize + offset3);
        vertices.push_back(y);
        vertices.push_back(currentSize + offset4);
        vertices.push_back(1.0f);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);

        // Top left - with variation
        vertices.push_back(-currentSize + offset4);
        vertices.push_back(y);
        vertices.push_back(currentSize + offset1);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);

        // Calculate indices for this layer
        unsigned int startIdx = layer * 4;
        indices.push_back(startIdx + 0);
        indices.push_back(startIdx + 1);
        indices.push_back(startIdx + 2);

        indices.push_back(startIdx + 0);
        indices.push_back(startIdx + 2);
        indices.push_back(startIdx + 3);
    }

    // Add many tiny billboard clouds that blend with the main layers
    const int numBillboards = 0; // More billboards for better blending
    std::uniform_real_distribution<float> posDist(-800.0f, 800.0f);
    std::uniform_real_distribution<float> heightDist(baseHeight - 10.0f, baseHeight + 60.0f);
    std::uniform_real_distribution<float> sizeDist(5.0f, 15.0f); // Much smaller sizes so they're barely visible
   
    // Starting index for billboards
    unsigned int startVertex = vertices.size() / 6;

    for (int i = 0; i < numBillboards; i++) {
        float x = posDist(gen);
        float z = posDist(gen);
        float y = heightDist(gen);  // Keep billboards at appropriate cloud height
        float size = sizeDist(gen);

        // Bottom left
        vertices.push_back(x - size);
        vertices.push_back(y - size);
        vertices.push_back(z);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);  // Billboard flag

        // Bottom right
        vertices.push_back(x + size);
        vertices.push_back(y - size);
        vertices.push_back(z);
        vertices.push_back(1.0f);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);

        // Top right
        vertices.push_back(x + size);
        vertices.push_back(y + size);
        vertices.push_back(z);
        vertices.push_back(1.0f);
        vertices.push_back(1.0f);
        vertices.push_back(1.0f);

        // Top left
        vertices.push_back(x - size);
        vertices.push_back(y + size);
        vertices.push_back(z);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back(1.0f);

        // Add indices for this billboard
        unsigned int billboardStartIdx = startVertex + i * 4;
        indices.push_back(billboardStartIdx + 0);
        indices.push_back(billboardStartIdx + 1);
        indices.push_back(billboardStartIdx + 2);

        indices.push_back(billboardStartIdx + 0);
        indices.push_back(billboardStartIdx + 2);
        indices.push_back(billboardStartIdx + 3);
    }

    // Set vertex and index counts
    vertexCount = vertices.size() / 6;
    indexCount = indices.size();

    // Create OpenGL buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord attribute (2 floats)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Billboard flag (1 float)
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void CloudSystem::Render(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos,
    GLuint cubemapTexture, float currentTime) {

    static bool firstRender = true;
    if (firstRender) {
        std::cout << "Rendering clouds with " << vertexCount << " vertices and " << indexCount << " indices" << std::endl;
        firstRender = false;
    }

    static Shader cloudShader("clouds.vert", "clouds.frag");

    // Configure blending for more realistic transparent clouds
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Disable depth writing for transparent objects

    // Use shader and set uniforms
    cloudShader.use();
    cloudShader.setMat4("projection", projection);
    cloudShader.setMat4("view", view);
    cloudShader.setVec3("cameraPos", cameraPos);
    cloudShader.setFloat("time", currentTime);

    // Set cloud appearance parameters
    cloudShader.setVec3("cloudColor", cloudColor);
    cloudShader.setVec3("cloudBaseColor", cloudBaseColor);
    cloudShader.setFloat("cloudCoverage", cloudCoverage);
    cloudShader.setFloat("cloudDensity", cloudDensity);
    cloudShader.setFloat("cloudSpeed", cloudSpeed);
    cloudShader.setVec3("windDirection", windDirection);

    // Bind cubemap for skybox sampling and cloud transparency
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    cloudShader.setInt("skybox", 0);

    // Set model matrix (centered on camera for XZ)
    glm::mat4 model = glm::mat4(1.0f);

    // Move clouds with camera on XZ plane for infinite effect
    model = glm::translate(model, glm::vec3(cameraPos.x, 0.0f, cameraPos.z));


    cloudShader.setMat4("model", model);

    // Render clouds
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restore state
    glDepthMask(GL_TRUE); // Re-enable depth writing
    glDisable(GL_BLEND);
}


void CloudSystem::AddBillboardClouds(std::vector<float>& vertices, std::vector<unsigned int>& indices, float baseHeight) {
    // Add some billboarded cloud puffs that will always face the camera
    const int numBillboards = 20;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDist(-400.0f, 400.0f);
    std::uniform_real_distribution<float> heightDist(baseHeight - 20.0f, baseHeight + 80.0f);
    std::uniform_real_distribution<float> sizeDist(50.0f, 150.0f);

    // Calculate the starting vertex index for billboards
    int startVertex = vertices.size() / 6;

    for (int i = 0; i < numBillboards; i++) {
        float x = posDist(gen);
        float z = posDist(gen);
        float y = heightDist(gen);
        float size = sizeDist(gen);

        // Add quad vertices for billboard
        // Each vertex has: x,y,z, u,v, billboard flag (1.0)

        // Bottom left
        vertices.push_back(x - size);
        vertices.push_back(y - size);
        vertices.push_back(z);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);  // Billboard flag

        // Rest of vertices...

        // Calculate indices for this billboard
        unsigned int startIdx = startVertex;
        indices.push_back(startIdx + 0);
        indices.push_back(startIdx + 1);
        indices.push_back(startIdx + 2);

        indices.push_back(startIdx + 0);
        indices.push_back(startIdx + 2);
        indices.push_back(startIdx + 3);

        startVertex += 4;
    }
}