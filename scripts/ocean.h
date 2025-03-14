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
    OceanFFTGenerator();
    ~OceanFFTGenerator();
    void GenerateSpectrums(int count);
struct SpectrumSettings;
 
std::vector<SpectrumSettings> spectrums;
 float gravity = 9.81;  // Gravity constant
 void spectrumBindBuffer(int location);
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
   GLuint spectrumBuffer;
};

OceanFFTGenerator::OceanFFTGenerator() {
    glGenBuffers(1, &spectrumBuffer);
 
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

    // Bind SSBO to binding point 0
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, location, spectrumBuffer);
}