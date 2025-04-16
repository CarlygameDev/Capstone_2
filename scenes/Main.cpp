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

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#define PI 3.14
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* path);
void load_VAO_VBO(unsigned int* vao, unsigned int* vbo, const float* vertices, const int* index);
unsigned int loadCubemap(vector<std::string> faces);
void load_Skybox(unsigned int* vao, unsigned int* vbo, unsigned int* cube_tex, vector<std::string> names);
float getShaderUniformFloat(const Shader& shader, const std::string& uniformName, float defaultValue);
void ShowTextureSettingsWindow(OceanFFTGenerator& oceanSettings, ComputeShader& spectrum, ComputeShader& conjugate);
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






std::vector<Layer> layers;



void DrawPerFrameSettings(
    ComputeShader& time_evolution,
    ComputeShader& normalizeFFT)
{
    ImGui::SetNextWindowSize(ImVec2(350, 300), ImGuiCond_Once);
    if (!ImGui::Begin("Per Frame Parameters")) {
        ImGui::End();
        return;
    }

    // === Speed of the Simulation ===
    if (ImGui::CollapsingHeader("Speed of the Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        static int speed = 1;
        static float repeatTime = 200.0f;

        if (ImGui::SliderInt("Speed", &speed, 0, 10)) {
            time_evolution.use();
            time_evolution.setInt("speed", speed);
        }

        if (ImGui::SliderFloat("Repeat Time", &repeatTime, 1.0f, 200.0f, "%.1f")) {
            time_evolution.use();
            time_evolution.setFloat("RepeatTime", repeatTime);
        }
    }

    // === Foam Simulation Parameters ===
    if (ImGui::CollapsingHeader("Foam Simulation Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        static glm::vec2 lambda = glm::vec2(1.0f, 1.0f);
        static float foamDecayRate = 0.0175f;
        static float foamBias = 0.85f;
        static float foamThreshold = 0.0f;
        static float foamAdd = 0.01f;

        if (ImGui::SliderFloat2("Lambda", glm::value_ptr(lambda), 0.0f, 5.0f, "%.2f")) {
            normalizeFFT.use();
            normalizeFFT.setVec2("_Lambda", lambda);
        }

        if (ImGui::SliderFloat("Foam Decay Rate", &foamDecayRate, 0.0f, 1.0f, "%.4f")) {
            normalizeFFT.use();
            normalizeFFT.setFloat("_FoamDecayRate", foamDecayRate);
        }

        if (ImGui::SliderFloat("Foam Bias", &foamBias, -1.0f, 1.0f, "%.2f")) {
            normalizeFFT.use();
            normalizeFFT.setFloat("_FoamBias", foamBias);
        }

        if (ImGui::SliderFloat("Foam Threshold", &foamThreshold, 0.0f, 1.0f, "%.2f")) {
            normalizeFFT.use();
            normalizeFFT.setFloat("_FoamThreshold", foamThreshold);
        }

        if (ImGui::SliderFloat("Foam Add", &foamAdd, 0.0f, 0.1f, "%.3f")) {
            normalizeFFT.use();
            normalizeFFT.setFloat("_FoamAdd", foamAdd);
        }
    }

    ImGui::End();
}
void DrawOceanSurfaceSettings(Shader& oceanShader)
{
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
    if (!ImGui::Begin("Ocean Surface Appearance")) {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Surface Appearance Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        static glm::vec3 sunIrradiance = glm::vec3(1.0f, 0.694f, 0.32f);
        static glm::vec3 scatterColor = glm::vec3(0.016f, 0.0736f, 0.16f);
        static glm::vec3 bubbleColor = glm::vec3(0.0f, 0.02f, 0.016f);
        static glm::vec3 foamColor = glm::vec3(0.9f, 0.9f, 1.0f);

        static float normalStrength = 1.0f;
        static float roughness = 0.075f;
        static float foamRoughnessModifier = 0.0f;
        static float environmentLightStrength = 0.5f;
        static float heightModifier = 1.0f;
        static float bubbleDensity = 1.0f;
        static float wavePeakScatterStrength = 1.0f;
        static float scatterStrength = 1.0f;
        static float scatterShadowStrength = 0.5f;
        static float displacementDepthAttenuation = 1.0f;
        static float underwaterFadeStrength = 2.0f;

        auto setColor = [&](const char* label, glm::vec3& value, const char* uniform) {
            if (ImGui::ColorEdit3(label, glm::value_ptr(value))) {
                oceanShader.use();
                oceanShader.setVec3(uniform, value);
            }
            };

        auto setSlider = [&](const char* label, float& value, float min, float max, const char* uniform) {
            if (ImGui::SliderFloat(label, &value, min, max, "%.3f")) {
                oceanShader.use();
                oceanShader.setFloat(uniform, value);
            }
            };

        setColor("Sun Irradiance", sunIrradiance, "_SunIrradiance");
        setColor("Scatter Color", scatterColor, "_ScatterColor");
        setColor("Bubble Color", bubbleColor, "_BubbleColor");
        setColor("Foam Color", foamColor, "_FoamColor");

        setSlider("Normal Strength", normalStrength, 0.0f, 2.0f, "_NormalStrength");
        setSlider("Roughness", roughness, 0.0f, 1.0f, "_Roughness");
        setSlider("Foam Roughness Modifier", foamRoughnessModifier, 0.0f, 1.0f, "_FoamRoughnessModifier");
        setSlider("Environment Light Strength", environmentLightStrength, 0.0f, 2.0f, "_EnvironmentLightStrength");
        setSlider("Height Modifier", heightModifier, 0.0f, 10.0f, "_HeightModifier");
        setSlider("Bubble Density", bubbleDensity, 0.0f, 1.0f, "_BubbleDensity");
        setSlider("Wave Peak Scatter Strength", wavePeakScatterStrength, 0.0f, 10.0f, "_WavePeakScatterStrength");
        setSlider("Scatter Strength", scatterStrength, 0.0f, 10.0f, "_ScatterStrength");
        setSlider("Scatter Shadow Strength", scatterShadowStrength, 0.0f, 1.0f, "_ScatterShadowStrength");
        setSlider("Displacement Depth Attenuation", displacementDepthAttenuation, 0.0f, 5.0f, "_DisplacementDepthAttenuation");
        setSlider("Underwater Fade Strength", underwaterFadeStrength, 0.0f, 5.0f, "_UnderwaterFadeStrength");
    }

    ImGui::End();
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
    OceanFFTGenerator oceanSettings(layers);
    ComputeShader spectrum("Spectrum_INIT.cps");
    ComputeShader conjugate("SpectrumConjugate.cps");
    
    oceanSettings.CalculateSpectrum(spectrum, conjugate);
    oceanSettings.createFFTWaterPlane(100);

    ComputeShader timeEvolutionShader("time_evolution.cps");
    timeEvolutionShader.use();


    ComputeShader horizontalFFT("horizontalFFT.cps");
    ComputeShader verticalFFT("verticalFFT.cps");

    Shader oceanShader("oceanFFT.vert", "oceanFFT.frag", nullptr, "oceanFFT.tcs", "oceanFFT.tes");
    oceanShader.use();

   
    oceanShader.setVec3("_lightDir",sunDirection);
    oceanShader.setInt("_EnvironmentMap",2);
    oceanShader.setInt("_SceneColor", 3);

    oceanShader.setInt("_DisplacementTextures", 0);
    oceanShader.setInt("_SlopeTextures", 1);

    ComputeShader normalizeFFT("fftNormalize.cps");
    normalizeFFT.use();



    Shader screenShader("PP.vert","PP.frag");
    screenShader.use();
    screenShader.setInt("screenTexture",0);
    screenShader.setInt("depthTexture", 1);
    screenShader.setInt("DisplacementTextures", 2);
    
  
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

    unsigned int depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Initialize ocean parameters from shader defaults
    oceanShader.use();
    

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
        oceanSettings.EvolveSpectrum(timeEvolutionShader);
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

    
     //ocean.Draw(oceanShader);
        //renderCube();

        oceanShader.use();
        oceanShader.setMat4("model", model);
        oceanShader.setMat4("inverse_model", glm::transpose(glm::inverse(glm::mat3(model))));
        oceanShader.setMat4("view", view);
        oceanShader.setMat4("projection", projection);
        oceanShader.setVec3("cameraPos", camera.Position);
        oceanShader.setInt("_TextureZ",oceanSettings.TextureCount());

       


   

        // Skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
         auto skybox_view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", skybox_view);

        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);


        oceanShader.use();
        model = glm::mat4();
        model = glm::scale(model, glm::vec3(1, 1, 1));
        oceanShader.setMat4("model", model);
        oceanShader.setMat4("inverse_model", glm::transpose(glm::inverse(glm::mat3(model))));
        
        oceanShader.setMat4("projection", projection);
        oceanShader.setMat4("view", view);
        oceanShader.setVec3("cameraPos", camera.Position);
        oceanShader.setFloat("_Time",currentFrame);
        oceanSettings.bindTextures();
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        oceanSettings.RenderOcean();


        // now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture

        // Final screen render (postprocess pass)

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        screenShader.use();
        screenShader.setVec3("cameraPos", camera.Position);
        screenShader.setInt("_TextureZ", oceanSettings.TextureCount());

        screenShader.setMat4("invViewProj", glm::inverse(projection * view));
        

        glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    
        glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, depthTexture);
      glActiveTexture(GL_TEXTURE2);
      glBindTexture(GL_TEXTURE_2D, oceanSettings.DisplacementTexture());
        renderQuad();

        // === IMGUI UI ===
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (cursorEnabled) {
            DrawPerFrameSettings(timeEvolutionShader,normalizeFFT);
            DrawOceanSurfaceSettings(oceanShader);
        }
        ShowTextureSettingsWindow(oceanSettings,spectrum,conjugate);

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
  


void ShowSpectrumSettings(DisplaySpectrumSettings& settings, const char* labelPrefix,const char* name)
{
    ImGui::Text(name);
    ImGui::NextColumn();

    ImGui::SliderFloat((std::string("Scale##") + labelPrefix).c_str(), &settings.scale, 0.0f, 5.0f);
    ImGui::SliderFloat((std::string("Wind Speed##") + labelPrefix).c_str(), &settings.windSpeed, 0.0f, 100.0f);
    ImGui::SliderFloat((std::string("Wind Direction##") + labelPrefix).c_str(), &settings.windDirection, 0.0f, 360.0f);
    ImGui::SliderFloat((std::string("Fetch##") + labelPrefix).c_str(), &settings.fetch, 0.0f, 10000.0f);
    ImGui::SliderFloat((std::string("Spread Blend##") + labelPrefix).c_str(), &settings.spreadBlend, 0.0f, 1.0f);
    ImGui::SliderFloat((std::string("Swell##") + labelPrefix).c_str(), &settings.swell, 0.0f, 1.0f);
    ImGui::SliderFloat((std::string("Peak Enhancement##") + labelPrefix).c_str(), &settings.peakEnhancement, 0.0f, 10.0f);
    ImGui::SliderFloat((std::string("Short Waves Fade##") + labelPrefix).c_str(), &settings.shortWavesFade, 0.0f, 1.0f);
}



void ShowTextureSettingsWindow(OceanFFTGenerator& oceanSettings, ComputeShader& spectrum,ComputeShader& conjugate)
{
    static int sliderValue = 4;
    static int selectedTextureIdx = 2;

    static int seed = 1;
    static float lowCutoff = 0.01f;
    static float highCutoff = 9000.0f;
    static float depth = 20.0f;
    static float gravity = 9.8f;

    static const char* textureSizes[] = {
        "2048", "1024", "512", "256", "128", "64", "32", "16"
    };
    static const int numTextureSizes = sizeof(textureSizes) / sizeof(textureSizes[0]);

    ImGui::Begin("Spectrum Settings");

    // Slider for texture count
    if (ImGui::SliderInt("Texture Count", &sliderValue, 1, 4)) {
        if (layers.size() < sliderValue)
            layers.resize(sliderValue);
        else if (layers.size() > sliderValue)
            layers.resize(sliderValue);
    }

    // Texture size selection
    if (ImGui::BeginCombo("Texture Size", textureSizes[selectedTextureIdx])) {
        for (int i = 0; i < numTextureSizes; i++) {
            bool isSelected = (selectedTextureIdx == i);
            if (ImGui::Selectable(textureSizes[i], isSelected))
                selectedTextureIdx = i;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // New parameters
    ImGui::SliderInt("Seed", &seed, 0, 1000000);
    ImGui::SliderFloat("Low Cutoff", &lowCutoff, 0.0001f, 9000.0f, "%.4f");
    ImGui::SliderFloat("High Cutoff", &highCutoff, 0.0001f, 9000.0f, "%.4f");
    ImGui::SliderFloat("Depth", &depth, 2.0f, 20.0f);
    ImGui::SliderFloat("Gravity", &gravity, 0.0f, 20.0f);

    // Show UI for each layer
    for (int i = 0; i < sliderValue; ++i) {
        std::string layerLabel = "Layer " + std::to_string(i + 1);
        if (ImGui::CollapsingHeader(layerLabel.c_str())) {
            ImGui::InputInt(("Domain Size##" + std::to_string(i)).c_str(), &layers[i].DomainSize);
            ShowSpectrumSettings(layers[i].spec1, ("Layer" + std::to_string(i) + "_Spec1").c_str(), "Spectrum 1");
            ShowSpectrumSettings(layers[i].spec2, ("Layer" + std::to_string(i) + "_Spec2").c_str(), "Spectrum 2");
        }
    }

    // Padding from edge
    float padding = 10.0f;
    float buttonWidth = 80.0f;

    // Position to bottom right
    float windowWidth = ImGui::GetWindowContentRegionMax().x;
    ImGui::SetCursorPosX(windowWidth - buttonWidth - padding);
    
    if (ImGui::Button("Bake", ImVec2(buttonWidth, 0))) {
        perChangeParameters parameters;
        parameters.TextureSize = std::stoi(textureSizes[selectedTextureIdx]);
        parameters.TextureCount = sliderValue;
        parameters.seed = seed;
        parameters.lowCutOff = lowCutoff;
        parameters.highCutOff = highCutoff;
        parameters.Depth = depth;
        parameters.Gravity = gravity;
        parameters.layers = layers; 

       oceanSettings. InitialBake(parameters);
       oceanSettings.CalculateSpectrum(spectrum, conjugate);
    }
    ImGui::End();
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






