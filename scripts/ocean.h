#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <glad/glad.h>
#include <glm/glm.hpp>


GLuint CreateTextureArray(int width, int height, int depth, GLenum format, bool useMips) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);

    // Determine number of mip levels
    GLint levels = useMips ? (1 + floor(log2(glm::max(width, height)))) : 1;

    // Allocate storage with mipmaps
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, levels, format, width, height, depth);

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,  GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set wrapping mode
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Anisotropic filtering
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);

    return textureID; 
}



struct DisplaySpectrumSettings {
public:
    //   [Range(0, 5)]
    float scale;
    float windSpeed;
    //  [Range(0.0f, 360.0f)]
    float windDirection;
    float fetch;
    //   [Range(0, 1)]
    float spreadBlend;
    //  [Range(0, 1)]
    float swell;
    float peakEnhancement;
    float shortWavesFade;
};
class OceanFFTGenerator
{
public:
    OceanFFTGenerator(int textureSize);
    ~OceanFFTGenerator();
    void GenerateSpectrums(int count);

 

 float gravity = 9.81;  // Gravity constant
 void spectrumBindBuffer(int location);
 void CalculateSpectrum(ComputeShader Spectrum, ComputeShader conjugate);
 void EvolveSpectrum();
 void IFFT(ComputeShader horizontal, ComputeShader vertical);
 void AssembleTextures(ComputeShader shader);
 void bindTextures();
 int const DisplacementTexture();
 int const SlopeTexture();
 void setDomain(ShaderBase shader);
void createFFTWaterPlane(const int SIZE);
void RenderOcean();
private:
   
    float RandomFloat(float min, float max);
    struct SpectrumSettings {  
        float scale;
        float angle;
        float spreadBlend;
        float swell;
        float alpha;
        float peakOmega;
        float gamma;
        float shortWavesFade;
    };
   float JonswapAlpha(float fetch, float windSpeed);
   float JonswapPeakFrequency(float fetch, float windSpeed);
   void FillSpectrumStruct(DisplaySpectrumSettings displaySettings, SpectrumSettings& computeSettings);
   GLuint N;//texture Size
   GLuint spectrumBuffer;
   GLuint initial_spectrumTextures;
   GLuint spectrumTextures;
   GLuint pingPongTextures;
   GLuint displacementTextures;
   GLuint slopeTextures;
   GLuint twiddleTexture;

   vector<int>DomainSizes;  
   std::vector<SpectrumSettings> spectrums;
   void debug();

   GLuint planeModel;
   GLuint indices;
  
};

OceanFFTGenerator::OceanFFTGenerator(int textureSize) {
    //          Spectrum Buffer
    ///////////////////////////////////
    glGenBuffers(1, &spectrumBuffer);
   
    
    //Textures 
 ///////////////////////////////////////
    //note does it need the F?
    initial_spectrumTextures = CreateTextureArray(textureSize, textureSize, 4, GL_RGBA16F, true);  // ARGBHalf in Unity
    spectrumTextures = CreateTextureArray(textureSize, textureSize, 8, GL_RGBA16F, true);     
     pingPongTextures   = CreateTextureArray(textureSize, textureSize, 8, GL_RGBA16F, true);            
    displacementTextures = CreateTextureArray(textureSize, textureSize, 4, GL_RGBA16F, true);     // ARGBHalf
    slopeTextures = CreateTextureArray(textureSize, textureSize, 4, GL_RG16F, true);              // RGHalf
   


    glGenTextures(1, &twiddleTexture);
    glBindTexture(GL_TEXTURE_2D, twiddleTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, log2(textureSize), textureSize); // log2(FFTSize) FFT stages, FFTSize entries

    // Set texture parameters (use NEAREST filtering to avoid interpolation)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Bind it to the compute shader
    glBindImageTexture(0, twiddleTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

    ComputeShader precomputeDiddy("precomputeDiddyFactor.cps");
    precomputeDiddy.use();
    precomputeDiddy.setInt("Size",textureSize);
    glDispatchCompute(log2(textureSize) , textureSize/2 / 8, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    N = textureSize;
    debug();
    return;

    for (int i = 0; i < 4;i++) {
   // DomainSizes.push_back((int)RandomFloat(100, 500));
        DomainSizes.push_back(94);
    }
   
}
OceanFFTGenerator::~OceanFFTGenerator() {}


void OceanFFTGenerator::GenerateSpectrums(int count)
{
    spectrums.clear();
    spectrums.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        SpectrumSettings spectrum;
        spectrum.scale = RandomFloat(0.1f, 5.0f);
        float windDirection = RandomFloat(0.0f, 360);
        spectrum.angle = windDirection/180* glm::pi<float>();
        spectrum.spreadBlend = RandomFloat(0.0f, 1.0f);
        spectrum.swell = glm::clamp<float>(RandomFloat(0.0f, 1.0f), 0.01f, 1);
        float fetch= RandomFloat(0.0f,1000000000);
        float windSpeed = RandomFloat(0.0f, 50);
        spectrum.alpha = JonswapAlpha(fetch, windSpeed);
        spectrum.peakOmega = JonswapPeakFrequency( fetch,  windSpeed);
        spectrum.gamma = RandomFloat(1.0f, 3.0f);
        spectrum.shortWavesFade = RandomFloat(0.1f, 1.0f);

        spectrums.push_back(spectrum);
    }
}

float OceanFFTGenerator::RandomFloat(float min, float max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}
float  OceanFFTGenerator::JonswapAlpha(float fetch, float windSpeed) {
    return 0.076f * glm::pow(gravity * fetch / windSpeed / windSpeed, -0.22f);
}

float  OceanFFTGenerator::JonswapPeakFrequency(float fetch, float windSpeed) {
    return 22 * glm::pow(windSpeed * fetch / gravity / gravity, -0.33f);
}

void OceanFFTGenerator::FillSpectrumStruct(DisplaySpectrumSettings displaySettings,  SpectrumSettings& computeSettings) {
    computeSettings.scale = displaySettings.scale;
    computeSettings.angle = displaySettings.windDirection / 180 * glm::pi<float>();
    computeSettings.spreadBlend = displaySettings.spreadBlend;
    computeSettings.swell = glm::clamp<float>(displaySettings.swell, 0.01f, 1);
    computeSettings.alpha = JonswapAlpha(displaySettings.fetch, displaySettings.windSpeed);
    computeSettings.peakOmega = JonswapPeakFrequency(displaySettings.fetch, displaySettings.windSpeed);
    computeSettings.gamma = displaySettings.peakEnhancement;
    computeSettings.shortWavesFade = displaySettings.shortWavesFade;
   
}
void OceanFFTGenerator::spectrumBindBuffer(int location) {

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, spectrumBuffer);

    
    glBufferData(GL_SHADER_STORAGE_BUFFER, spectrums.size() * sizeof(SpectrumSettings), spectrums.data(), GL_DYNAMIC_DRAW);

    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, spectrumBuffer);
}

void OceanFFTGenerator::CalculateSpectrum(ComputeShader Spectrum, ComputeShader conjugate ) {
 
    Spectrum.use();  
 spectrumBindBuffer(1);
   
    setDomain(Spectrum);
    glBindImageTexture(0, initial_spectrumTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
    glDispatchCompute(N / 16,  N/ 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    conjugate.use();
   
  
    glDispatchCompute(N / 16, N / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void OceanFFTGenerator::EvolveSpectrum() {

    glBindImageTexture(0, initial_spectrumTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
    glBindImageTexture(1, spectrumTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
    glDispatchCompute(N / 16, N / 16, DomainSizes.size());
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  
}
void OceanFFTGenerator::IFFT(ComputeShader horizontal, ComputeShader vertical) {
    int logSize = (int)log2(N);
    bool pingPong = false;

   

    glBindImageTexture(0, spectrumTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
    glBindImageTexture(1, pingPongTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
    glBindImageTexture(2, twiddleTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
    horizontal.use();
    int depth = DomainSizes.size() * 2;

    for (int i = 0; i < logSize; i++)
    {
        pingPong = !pingPong;
        horizontal.setInt("Step", i);
        horizontal.setBool("PingPong", pingPong);
        glDispatchCompute( N / 8, N / 8, depth);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    vertical.use();

    for (int i = 0; i < logSize; i++)
    {
        pingPong = !pingPong;
        vertical.setInt("Step", i);
        vertical.setBool("PingPong", pingPong);
        glDispatchCompute(N / 8, N / 8,depth );
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    if (pingPong)
    {
        glCopyImageSubData(
            pingPongTextures, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,  // Source
            spectrumTextures, GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0,  // Destination
            N, N, DomainSizes.size()*2  // Copy full texture array
        );

    }

    

   
}
void OceanFFTGenerator::AssembleTextures(ComputeShader shader) {
    shader.use();
    glBindImageTexture(0, spectrumTextures, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
    glBindImageTexture(1, displacementTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
    glBindImageTexture(2, slopeTextures, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG16F);
    glDispatchCompute(N / 16, N / 16, DomainSizes.size());
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

 

    
}
void  OceanFFTGenerator::setDomain(ShaderBase shader) {
    glUniform1iv(glGetUniformLocation(shader.ID, "domains"), DomainSizes.size(), DomainSizes.data());
}
int const OceanFFTGenerator::DisplacementTexture () {
    return displacementTextures;
}
int const OceanFFTGenerator:: SlopeTexture() {
    return slopeTextures;
}
void OceanFFTGenerator::bindTextures() {

glBindTexture(GL_TEXTURE_2D_ARRAY, displacementTextures);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, slopeTextures);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
 //   glBindTexture(GL_TEXTURE_2D_ARRAY, initial_spectrumTextures);
 //   glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, displacementTextures);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, slopeTextures);
    //glActiveTexture(GL_TEXTURE3);
    //glBindTexture(GL_TEXTURE_2D_ARRAY, spectrumTextures);
    //glActiveTexture(GL_TEXTURE4);
    //glBindTexture(GL_TEXTURE_2D_ARRAY, initial_spectrumTextures);
    //glActiveTexture(GL_TEXTURE5);
    //glBindTexture(GL_TEXTURE_2D_ARRAY, twiddleTexture);
  
}
void OceanFFTGenerator::RenderOcean() {
    glPatchParameteri(GL_PATCH_VERTICES, 4);
       glBindVertexArray(planeModel);
       glDrawElements(GL_PATCHES, indices, GL_UNSIGNED_INT, 0);
}

void OceanFFTGenerator::debug() {
    DisplaySpectrumSettings settings[8];

    // Spectrum 1
    settings[0].scale = 0.1f;
    settings[0].windSpeed = 2.0f;
    settings[0].windDirection = 22.0f; // Assuming default value since it's not provided
    settings[0].fetch = 100000.0f;
    settings[0].spreadBlend = 0.642f; // Assuming default value since it's not provided
    settings[0].swell = 1.0f;
    settings[0].peakEnhancement = 1.0f;
    settings[0].shortWavesFade = 0.025f;

    // Spectrum 2
    // Assuming default values for missing fields
    settings[1].scale = 0.07f; // Not provided
    settings[1].windSpeed = 2.0f; 
    settings[1].windDirection = 59.0f; // Not provided
    settings[1].fetch = 1000; // Not provided
    settings[1].spreadBlend = 0.0f; // Not provided
    settings[1].swell = 1.0f; // Not provided
    settings[1].peakEnhancement = 1.0f; // Not provided
    settings[1].shortWavesFade = 0.01f; // Not provided

    // Spectrum 3
    settings[2].scale = 0.25f;
    settings[2].windSpeed = 20.0f;
    settings[2].windDirection = 97.0f; // Assuming default value since it's not provided
    settings[2].fetch = 1e+08f;
    settings[2].spreadBlend = 0.14;
    settings[2].swell = 1;
    settings[2].peakEnhancement = 1.0f;
    settings[2].shortWavesFade = 0.5f;

    // Spectrum 4
    settings[3].scale = 0.25f;
    settings[3].windSpeed = 20.0f;
    settings[3].windDirection = 67.0f; // Assuming default value since it's not provided
    settings[3].fetch = 1000000.0f;
    settings[3].spreadBlend = 0.47f;
    settings[3].swell = 1.0f;
    settings[3].peakEnhancement = 1.0f;
    settings[3].shortWavesFade = 0.5f;

    // Spectrum 5
    settings[4].scale = 0.15f;
    settings[4].windSpeed = 5.0f;
    settings[4].windDirection = 105.0f; // Assuming default value since it's not provided
    settings[4].fetch = 1000000.0f;
    settings[4].spreadBlend = 0.2f; // Assuming default value since it's not provided
    settings[4].swell = 1.0f;
    settings[4].peakEnhancement = 1.0f;
    settings[4].shortWavesFade = 0.5f;

    // Spectrum 6
    settings[5].scale = 0.1f;
    settings[5].windSpeed = 1.0f; // Not provided
    settings[5].windDirection = 19.0f; // Not provided
    settings[5].fetch = 10000.0f;
    settings[5].spreadBlend = 0.298f; // Not provided
    settings[5].swell = 0.695f;
    settings[5].peakEnhancement = 1.0f;
    settings[5].shortWavesFade = 0.5f;

    // Spectrum 7
    settings[6].scale = 1.00f;
    settings[6].windSpeed = 1.0f; // Not provided
    settings[6].windDirection = 209.0f; // Not provided
    settings[6].fetch = 200000.0f;
    settings[6].spreadBlend = 0.56f; // Not provided
    settings[6].swell = 1.0f;
    settings[6].peakEnhancement = 1.0f;
    settings[6].shortWavesFade = 0.0001f;

    // Spectrum 8
    settings[7].scale = 0.23f;
    settings[7].windSpeed = 1.0f; // Not provided
    settings[7].windDirection = 0.0f; // Not provided
    settings[7].fetch = 1000.0f;
    settings[7].spreadBlend = 0.0f; // Not provided
    settings[7].swell = 0.0f;
    settings[7].peakEnhancement = 1.0f;
    settings[7].shortWavesFade = 0.0001f;

    // Now you can use the settings array for debugging or further processing
    for (int i = 0; i < 8; ++i) {
        SpectrumSettings computeSettings;
        FillSpectrumStruct(settings[i], computeSettings);
        spectrums.push_back(computeSettings);
    }
    DomainSizes = { 94,128,64,32 };
}



void OceanFFTGenerator::createFFTWaterPlane(const int SIZE) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Generate vertices
    for (int z = 0; z < SIZE; ++z) {
        for (int x = 0; x < SIZE; ++x) {
            float xPos = (x - SIZE / 2)*5;
            float zPos = (z - SIZE / 2)*5;

            vertices.push_back(xPos);
            vertices.push_back(0);
            vertices.push_back(zPos);
        }
    }

    //// Generate indices for a grid
    //for (int z = 0; z < SIZE - 1; ++z) {
    //    for (int x = 0; x < SIZE - 1; ++x) {
    //        int topLeft = z * SIZE + x;
    //        int topRight = topLeft + 1;
    //        int bottomLeft = (z + 1) * SIZE + x;
    //        int bottomRight = bottomLeft + 1;

    //        // First triangle
    //        indices.push_back(topLeft);
    //        indices.push_back(bottomLeft);
    //        indices.push_back(topRight);

    //        // Second triangle
    //        indices.push_back(topRight);
    //        indices.push_back(bottomLeft);
    //        indices.push_back(bottomRight);
    //    }
    //}
    
    // Generate indices for quad patches (4 vertices per quad)
    for (int z = 0; z < SIZE - 1; ++z) {
        for (int x = 0; x < SIZE - 1; ++x) {
            int topLeft = z * SIZE + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * SIZE + x;
            int bottomRight = bottomLeft + 1;

            // Expected order: bottom-left, bottom-right, top-left, top-right
            indices.push_back(bottomLeft);  // gl_in[0]
            indices.push_back(bottomRight); // gl_in[1]
            indices.push_back(topLeft);     // gl_in[2]
            indices.push_back(topRight);    // gl_in[3]


        }
    }

    // Upload to OpenGL
    glGenVertexArrays(1, &planeModel);
    glBindVertexArray(planeModel);

    GLuint VBO, EBO;
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Define vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Store the number of indices for rendering
    this->indices = indices.size();
}
