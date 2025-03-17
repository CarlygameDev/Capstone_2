#pragma once
#include <iostream>
#include <vector>
#include <random>
#include <glad/glad.h>
#include <glm/glm.hpp>

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
 
 void setDomain(ShaderBase shader);
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
   void FillSpectrumStruct(DisplaySpectrumSettings displaySettings, SpectrumSettings computeSettings);
   GLuint N;//texture Size
   GLuint spectrumBuffer;
   GLuint initial_spectrumTextures;
   GLuint spectrumTextures;
   GLuint displacementTextures;
   GLuint slopeTextures;
   vector<int>DomainSizes;  
   std::vector<SpectrumSettings> spectrums;
};

OceanFFTGenerator::OceanFFTGenerator(int textureSize) {
    //          Spectrum Buffer
    ///////////////////////////////////
    glGenBuffers(1, &spectrumBuffer);
   
    
    //Textures 
 ///////////////////////////////////////
    glGenTextures(1, &initial_spectrumTextures);
    glBindTexture(GL_TEXTURE_2D_ARRAY, initial_spectrumTextures);

    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, textureSize, textureSize, 4);//hardcoded amount for now
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


    glGenTextures(1, &spectrumTextures);
    glBindTexture(GL_TEXTURE_2D_ARRAY, spectrumTextures);

    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, textureSize, textureSize, 4*2);//hardcoded amount for now
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &displacementTextures);
    glBindTexture(GL_TEXTURE_2D_ARRAY, displacementTextures);

    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, textureSize, textureSize, 4);//hardcoded amount for now
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenTextures(1, &slopeTextures);
    glBindTexture(GL_TEXTURE_2D_ARRAY, slopeTextures);

    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RG32F, textureSize, textureSize, 4);//hardcoded amount for now
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    for (int i = 0; i < 4;i++) {
   // DomainSizes.push_back((int)RandomFloat(100, 500));
        DomainSizes.push_back(94);
    }
    N = textureSize;
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

void OceanFFTGenerator::FillSpectrumStruct(DisplaySpectrumSettings displaySettings,  SpectrumSettings computeSettings) {
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
    glBindImageTexture(0, initial_spectrumTextures, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glDispatchCompute(N / 16,  N/ 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    conjugate.use();
   
    glBindImageTexture(0, spectrumTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
    glDispatchCompute(N / 16, N / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void OceanFFTGenerator::EvolveSpectrum() {

    glBindImageTexture(0, initial_spectrumTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(1, spectrumTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
    glDispatchCompute(N / 16, N / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  
}
void OceanFFTGenerator::IFFT(ComputeShader horizontal, ComputeShader vertical) {
    horizontal.use();



    glBindImageTexture(0, spectrumTextures, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
 
    glDispatchCompute(1, N, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);




    vertical.use();


    glDispatchCompute(1, N, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void OceanFFTGenerator::AssembleTextures(ComputeShader shader) {
    shader.use();
    glBindImageTexture(0, spectrumTextures, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, displacementTextures, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(2, slopeTextures, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RG32F);
    glDispatchCompute(N/ 16, N / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
void  OceanFFTGenerator::setDomain(ShaderBase shader) {
    glUniform1iv(glGetUniformLocation(shader.ID, "domains"), DomainSizes.size(), DomainSizes.data());
}

void OceanFFTGenerator::bindTextures() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, displacementTextures);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, slopeTextures);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D_ARRAY, spectrumTextures);
}
