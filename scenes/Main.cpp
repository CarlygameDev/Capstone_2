#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Shader.h>
#include <iostream>
#include "stb/stb_image.h"
#include <fileFinder.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <camera.h>
#include <Model.h>
#include <ocean.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#define PI 3.14

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* path);
void load_VAO_VBO(unsigned int* vao, unsigned int* vbo, const float* vertices, const int* index);
unsigned int loadCubemap(vector<std::string> faces);
void load_Skybox(unsigned int* vao, unsigned int* vbo, unsigned int* cube_tex, vector<std::string> names);
float getShaderUniformFloat(const Shader& shader, const std::string& uniformName, float defaultValue);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void renderScene(const Shader& shader);
void renderCube();
void renderQuad();
void renderSphere();
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));

bool firstMouse = true;
bool shadows = true;
bool shadowsKeyPressed = false;
bool cursorEnabled = false;

float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
unsigned int planeVAO;

// Define the ocean parameters struct at global scope
struct OceanParams {
    float bubbleDensity = 1.0f;
    float roughness = 0.075f;
    float foamRoughnessModifier = 0.0f;
    float heightModifier = 1.0f;
    float wavePeakScatterStrength = 1.0f;
    float scatterStrength = 1.0f;
    float scatterShadowStrength = 0.5f;
    float environmentLightStrength = 0.5f;
    glm::vec3 sunIrradiance = glm::vec3(1.0f, 0.64f, 0.32f);
    glm::vec3 bubbleColor = glm::vec3(0.0f, 0.02f, 0.016f);
    glm::vec3 scatterColor = glm::vec3(0.016f, 0.07359998f, 0.16f);

};

struct FoamSettings {
    glm::vec3 foam = glm::vec3(1.0f, 1.0f, 1.0f);
    float foamBias = 0.855f;
    float foamThreshold = 0.0f;
    float foamAdd = 0.01f;
    float foamDecayRate = 0.0175f;
    float foamDepthFalloff = 1.0f;
} foamSettings;



// Declare the instance at global scope
OceanParams oceanParams;

// Helper function to get uniform float values from shader
float getShaderUniformFloat(const Shader& shader, const std::string& uniformName, float defaultValue) {
    GLint location = glGetUniformLocation(shader.ID, uniformName.c_str());
    if (location == -1) {
        return defaultValue; // Uniform not found
    }

    float value;
    glGetUniformfv(shader.ID, location, &value);
    return value;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSwapInterval(0);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader textureLoad("vTexture.vert", "vTexture.frag");
    Shader skyboxShader("skybox.vert", "skybox.frag");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    float scale_factor = 100;
    textureLoad.use();
    textureLoad.setFloat("scaleFactor", scale_factor);
    textureLoad.setInt("skybox", 5);
    glm::vec3 sunDirection = glm::vec3(-0.2f, -1.0f, -0.9f);
    glm::vec3 sunColor = glm::vec3(1.0, 0.7, 0.4);
    textureLoad.setVec3("sunColor", sunColor);
    textureLoad.setVec3("sunDirection", sunDirection);

    vector<std::string> faces = {
        fileFinder::getTexture("mountain_skybox/right.jpg"),
        fileFinder::getTexture("mountain_skybox/left.jpg"),
        fileFinder::getTexture("mountain_skybox/top.jpg"),
        fileFinder::getTexture("mountain_skybox/bottom.jpg"),
        fileFinder::getTexture("mountain_skybox/front.jpg"),
        fileFinder::getTexture("mountain_skybox/back.jpg")
    };

    unsigned int skyboxVAO, skyboxVBO, cubemapTexture;
    load_Skybox(&skyboxVAO, &skyboxVBO, &cubemapTexture, faces);
    skyboxShader.use();
    skyboxShader.setVec3("sunDirection", sunDirection);
    skyboxShader.setVec3("sunColor", sunColor);

    ComputeShader spectrum("Spectrum_INIT.cps");
    ComputeShader conjugate("SpectrumConjugate.cps");
    OceanFFTGenerator oceanSettings(1024);
    oceanSettings.CalculateSpectrum(spectrum, conjugate);
    oceanSettings.createFFTWaterPlane(100);

    ComputeShader timeEvolutionShader("time_evolution.cps");
    timeEvolutionShader.use();
    oceanSettings.setDomain(timeEvolutionShader);

    ComputeShader horizontalFFT("horizontalFFT.cps");
    ComputeShader verticalFFT("verticalFFT.cps");

    Shader oceanShader("oceanFFT.vert", "oceanFFT.frag", nullptr, "oceanFFT.tcs", "oceanFFT.tes");
    oceanShader.use();
    oceanShader.setVec3("_lightDir", sunDirection);
    oceanShader.setInt("_EnvironmentMap", 2);
    oceanShader.setInt("_DisplacementTextures", 0);
    oceanShader.setInt("_SlopeTextures", 1);

    ComputeShader normalizeFFT("fftNormalize.cps");
    normalizeFFT.use();

    Shader screenShader("PP.vert", "PP.frag");

    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    unsigned int textureColorbuffer;
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Initialize ocean parameters from shader defaults
    oceanShader.use();
    oceanParams.bubbleDensity = getShaderUniformFloat(oceanShader, "_BubbleDensity", 1.0f);
    oceanParams.roughness = getShaderUniformFloat(oceanShader, "_Roughness", 0.075f);
    oceanParams.foamRoughnessModifier = getShaderUniformFloat(oceanShader, "_FoamRoughnessModifier", 0.0f);
    oceanParams.heightModifier = getShaderUniformFloat(oceanShader, "_HeightModifier", 1.0f);
    oceanParams.wavePeakScatterStrength = getShaderUniformFloat(oceanShader, "_WavePeakScatterStrength", 1.0f);
    oceanParams.scatterStrength = getShaderUniformFloat(oceanShader, "_ScatterStrength", 1.0f);
    oceanParams.scatterShadowStrength = getShaderUniformFloat(oceanShader, "_ScatterShadowStrength", 0.5f);
    oceanParams.environmentLightStrength = getShaderUniformFloat(oceanShader, "_EnvironmentLightStrength", 0.5f);

    int fCounter = 0;
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (fCounter++ > 500) {
            std::cout << "FPS: " << 1.0f / deltaTime << std::endl;
            fCounter = 0;
        }

        processInput(window);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        // === Ocean Spectrum Update ===
        timeEvolutionShader.use();
        timeEvolutionShader.setFloat("time", currentFrame);
        oceanSettings.EvolveSpectrum();
        oceanSettings.IFFT(horizontalFFT, verticalFFT);
        oceanSettings.AssembleTextures(normalizeFFT);

        // === Main Render Pass ===
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 5000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        textureLoad.use();
        textureLoad.setMat4("model", model);
        textureLoad.setMat4("view", view);
        textureLoad.setMat4("projection", projection);
        textureLoad.setInt("textureArray", 0);

        oceanShader.use();
        oceanShader.setMat4("model", model);
        oceanShader.setMat4("inverse_model", glm::transpose(glm::inverse(glm::mat3(model))));
        oceanShader.setMat4("view", view);
        oceanShader.setMat4("projection", projection);
        oceanShader.setVec3("cameraPos", camera.Position);

        // Pass the ocean parameters to the shader
        oceanShader.setFloat("_BubbleDensity", oceanParams.bubbleDensity);
        oceanShader.setFloat("_Roughness", oceanParams.roughness);
        oceanShader.setFloat("_FoamRoughnessModifier", oceanParams.foamRoughnessModifier);
        oceanShader.setFloat("_HeightModifier", oceanParams.heightModifier);
        oceanShader.setFloat("_WavePeakScatterStrength", oceanParams.wavePeakScatterStrength);
        oceanShader.setFloat("_ScatterStrength", oceanParams.scatterStrength);
        oceanShader.setFloat("_ScatterShadowStrength", oceanParams.scatterShadowStrength);
        oceanShader.setFloat("_EnvironmentLightStrength", oceanParams.environmentLightStrength);
        oceanShader.setVec3("_SunIrradiance", oceanParams.sunIrradiance);
        oceanShader.setVec3("_BubbleColor", oceanParams.bubbleColor);
        oceanShader.setVec3("_ScatterColor", oceanParams.scatterColor);

        // Pass foam settings to the shader
        oceanShader.setVec3("_FoamColor", foamSettings.foam);
        oceanShader.setFloat("_FoamBias", foamSettings.foamBias);
        oceanShader.setFloat("_FoamThreshold", foamSettings.foamThreshold);
        oceanShader.setFloat("_FoamAdd", foamSettings.foamAdd);
        oceanShader.setFloat("_FoamDecayRate", foamSettings.foamDecayRate);
        oceanShader.setFloat("_FoamDepthFalloff", foamSettings.foamDepthFalloff);


        oceanSettings.bindTextures();
        oceanSettings.RenderOcean();

        // Skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        skyboxShader.setMat4("view", glm::mat4(glm::mat3(camera.GetViewMatrix())));
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        // Final screen render (postprocess pass)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        screenShader.use();
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        renderQuad();

        // === IMGUI UI ===
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (cursorEnabled) {
            ImGui::SetNextWindowSize(ImVec2(450, 400), ImGuiCond_Once);
            if (ImGui::Begin("Ocean Simulation Controls")) {
                if (ImGui::CollapsingHeader("PBR Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 100.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 20.0f);

                    ImGui::Columns(2, nullptr, false);

                    ImGui::Text("Sun Irradiance");
                    ImGui::NextColumn();
                    if (ImGui::ColorEdit3("##SunIrradiance", glm::value_ptr(oceanParams.sunIrradiance))) {
                        oceanShader.use();
                        oceanShader.setVec3("_SunIrradiance", oceanParams.sunIrradiance);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Bubble Color");
                    ImGui::NextColumn();
                    if (ImGui::ColorEdit3("##BubbleColor", glm::value_ptr(oceanParams.bubbleColor))) {
                        oceanShader.use();
                        oceanShader.setVec3("_BubbleColor", oceanParams.bubbleColor);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Scatter Color");
                    ImGui::NextColumn();
                    if (ImGui::ColorEdit3("##ScatterColor", glm::value_ptr(oceanParams.scatterColor))) {
                        oceanShader.use();
                        oceanShader.setVec3("_ScatterColor", oceanParams.scatterColor);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Bubble Density");
                    ImGui::NextColumn();
                    if (ImGui::SliderFloat("##BubbleDensity", &oceanParams.bubbleDensity, 0.0f, 1.0f, "%.2f")) {
                        oceanShader.use();
                        oceanShader.setFloat("_BubbleDensity", oceanParams.bubbleDensity);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Roughness");
                    ImGui::NextColumn();
                    if (ImGui::SliderFloat("##Roughness", &oceanParams.roughness, 0.0f, 1.0f, "%.2f")) {
                        oceanShader.use();
                        oceanShader.setFloat("_Roughness", oceanParams.roughness);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Foam Roughness Mod");
                    ImGui::NextColumn();
                    if (ImGui::SliderFloat("##FoamRoughness", &oceanParams.foamRoughnessModifier, 0.0f, 1.0f, "%.2f")) {
                        oceanShader.use();
                        oceanShader.setFloat("_FoamRoughnessModifier", oceanParams.foamRoughnessModifier);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Height Modifier");
                    ImGui::NextColumn();
                    if (ImGui::SliderFloat("##HeightModifier", &oceanParams.heightModifier, 0.0f, 10.0f, "%.2f")) {
                        oceanShader.use();
                        oceanShader.setFloat("_HeightModifier", oceanParams.heightModifier);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Wave Scatter Strength");
                    ImGui::NextColumn();
                    if (ImGui::SliderFloat("##WaveScatter", &oceanParams.wavePeakScatterStrength, 0.0f, 10.0f, "%.2f")) {
                        oceanShader.use();
                        oceanShader.setFloat("_WavePeakScatterStrength", oceanParams.wavePeakScatterStrength);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Scatter Strength");
                    ImGui::NextColumn();
                    if (ImGui::SliderFloat("##ScatterStrength", &oceanParams.scatterStrength, 0.0f, 10.0f, "%.2f")) {
                        oceanShader.use();
                        oceanShader.setFloat("_ScatterStrength", oceanParams.scatterStrength);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Scatter Shadow Strength");
                    ImGui::NextColumn();
                    if (ImGui::SliderFloat("##ScatterShadowStrength", &oceanParams.scatterShadowStrength, 0.0f, 1.0f, "%.2f")) {
                        oceanShader.use();
                        oceanShader.setFloat("_ScatterShadowStrength", oceanParams.scatterShadowStrength);
                    }
                    ImGui::NextColumn();

                    ImGui::Text("Environment Light Strength");
                    ImGui::NextColumn();
                    if (ImGui::SliderFloat("##EnvLightStrength", &oceanParams.environmentLightStrength, 0.0f, 1.0f, "%.2f")) {
                        oceanShader.use();
                        oceanShader.setFloat("_EnvironmentLightStrength", oceanParams.environmentLightStrength);
                    }
                    ImGui::Columns(1);
                    ImGui::PopStyleVar(2);
                }
                // === Foam Settings ===
                if (ImGui::CollapsingHeader("Foam Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 100.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 20.0f);

                    ImGui::Columns(2, nullptr, false); // Two-column layout

                    // Foam Color
                    ImGui::Text("Foam Color"); ImGui::NextColumn();
                    if (ImGui::ColorEdit3("##FoamColor", glm::value_ptr(foamSettings.foam))) {
                        oceanShader.use();
                        oceanShader.setVec3("_FoamColor", foamSettings.foam);
                    }
                    ImGui::NextColumn();

                    // Foam Bias
                    ImGui::Text("Foam Bias"); ImGui::NextColumn();
                    if (ImGui::SliderFloat("##FoamBias", &foamSettings.foamBias, -1.0f, 1.0f, "%.2f")) {
                        normalizeFFT.use();
                        normalizeFFT.setFloat("_FoamBias", foamSettings.foamBias);
                    }
                    ImGui::NextColumn();

                    // Foam Threshold
                    ImGui::Text("Foam Threshold"); ImGui::NextColumn();
                    if (ImGui::SliderFloat("##FoamThreshold", &foamSettings.foamThreshold, 0.0f, 1.0f, "%.2f")) {
                        normalizeFFT.use();
                        normalizeFFT.setFloat("_FoamThreshold", foamSettings.foamThreshold);
                    }
                    ImGui::NextColumn();

                    // Foam Add
                    ImGui::Text("Foam Add"); ImGui::NextColumn();
                    if (ImGui::SliderFloat("##FoamAdd", &foamSettings.foamAdd, 0.0f, 0.5f, "%.2f")) {
                        normalizeFFT.use();
                        normalizeFFT.setFloat("_FoamAdd", foamSettings.foamAdd);
                    }
                    ImGui::NextColumn();

                    // Foam Decay Rate
                    ImGui::Text("Foam Decay Rate"); ImGui::NextColumn();
                    if (ImGui::SliderFloat("##FoamDecayRate", &foamSettings.foamDecayRate, 0.0f, 1.0f, "%.2f")) {
                        normalizeFFT.use();
                        normalizeFFT.setFloat("_FoamDecayRate", foamSettings.foamDecayRate);
                    }
                    ImGui::NextColumn();

                    // Foam Depth Falloff
                    ImGui::Text("Foam Depth Falloff"); ImGui::NextColumn();
                    if (ImGui::SliderFloat("##FoamDepthFalloff", &foamSettings.foamDepthFalloff, 0.0f, 10.0f, "%.2f")) {
                        normalizeFFT.use();
                        normalizeFFT.setFloat("_FoamDepthFalloff", foamSettings.foamDepthFalloff);
                    }
                    ImGui::NextColumn();

                    ImGui::Columns(1); // Reset to single-column
                    ImGui::PopStyleVar(2);
                }



                ImGui::End();
            }
        }


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
  

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Cursor toggle logic
    static bool oKeyPressed = false;   // Track o key state

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !oKeyPressed)
    {
        cursorEnabled = !cursorEnabled;
        glfwSetInputMode(window, GLFW_CURSOR, cursorEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        oKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
    {
        oKeyPressed = false;
    }

    // Only process camera movement if the cursor is disabled (i.e., in FPS mode)
    if (!cursorEnabled)
    {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    // Toggle shadows with SPACE key
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !shadowsKeyPressed)
    {
        shadows = !shadows;
        shadowsKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        shadowsKeyPressed = false;
    }
}




// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    if (cursorEnabled)
        return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}





// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format = nrChannels == 4 ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void load_VAO_VBO(unsigned int* vao, unsigned int* vbo, const float* vertices,const int* index) {
 
    size_t length = sizeof(vertices);
    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);
    glBindVertexArray(*vao);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, length, &vertices, GL_STATIC_DRAW);
    for (int i = 0;i < sizeof(index);i++) {
        int start = i < 1 ? 0 : *(index + i - 1);
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(0, *(index+i), GL_FLOAT, GL_FALSE, length * sizeof(float), (void*)(start*sizeof(float)));
      
    }
}
void load_Skybox(unsigned int* vao, unsigned int* vbo, unsigned int* cube_tex, vector<std::string> names) {
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    
    glGenVertexArrays(1, vao);
    glGenBuffers(1, vbo);
    glBindVertexArray(*vao);
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    *cube_tex=loadCubemap(names);
}

// renders the 3D scene
// --------------------
// renders the 3D scene
// --------------------
void renderScene(const Shader& shader)
{
    // room cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(5.0f));
    shader.setMat4("model", model);
    glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
    shader.setInt("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
    renderCube();
    shader.setInt("reverse_normals", 0); // and of course disable it
    glEnable(GL_CULL_FACE);
    // cubes
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 3.0f, 1.0));
    model = glm::scale(model, glm::vec3(0.75f));
    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, -1.0f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 1.0f, 1.5));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.75f));
    shader.setMat4("model", model);
    renderCube();
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
             // bottom face
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
              1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
              1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
             -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             // top face
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
              1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
              1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
              1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
       // const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}






