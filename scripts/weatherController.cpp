#include "weatherController.h"
#include <iostream>

WeatherController::WeatherController(CloudSystem* cloudSys, RainSystem* rainSys)
    : cloudSystem(cloudSys), rainSystem(rainSys), 
      currentState(CLEAR), targetState(CLEAR),
      transitionTimer(0.0f), transitionDuration(3.0f),
      windDirection(glm::vec3(1.0f, 0.0f, 0.0f))
{
    // Initialize systems with clear weather
    WeatherParams params = GetParamsForState(CLEAR);
    cloudSystem->cloudCoverage = params.cloudCoverage;
    cloudSystem->cloudDensity = params.cloudDensity;
    rainSystem->rainIntensity = params.rainIntensity;
    
    // Set initial wind direction
    SetWindDirection(windDirection);
}

void WeatherController::Update(float deltaTime) {
    // Update transition if in progress
    if (currentState != targetState) {
        UpdateTransition(deltaTime);
    }
    
    // Always ensure wind direction is synchronized
    SetWindDirection(windDirection);
}

void WeatherController::SetWeatherState(WeatherState state) {
    // Only change if it's a new state
    if (state != currentState) {
        targetState = state;
        transitionTimer = 0.0f;
        
        // Print the current weather state
        std::string stateName;
        switch (state) {
            case CLEAR: stateName = "Clear"; break;
            case PARTLY_CLOUDY: stateName = "Partly Cloudy"; break;
            case CLOUDY: stateName = "Cloudy"; break;
            case LIGHT_RAIN: stateName = "Light Rain"; break;
            case HEAVY_RAIN: stateName = "Heavy Rain"; break;
        }
        std::cout << "Weather changing to: " << stateName << std::endl;
    }
}

void WeatherController::CycleWeatherState() {
    // Cycle to the next weather state
    int nextState = (static_cast<int>(currentState) + 1) % 5;
    SetWeatherState(static_cast<WeatherState>(nextState));
}

void WeatherController::SetWindDirection(const glm::vec3& direction) {
    windDirection = glm::normalize(direction);
    cloudSystem->windDirection = windDirection;
    rainSystem->windDirection = windDirection;
}

WeatherController::WeatherParams WeatherController::GetParamsForState(WeatherState state) {
    WeatherParams params;
    
    switch (state) {
        case CLEAR:
            params.cloudCoverage = 0.0f;
            params.cloudDensity = 0.0f;
            params.rainIntensity = 0.0f;
            break;
            
        case PARTLY_CLOUDY:
            params.cloudCoverage = 0.4f;
            params.cloudDensity = 0.7f;
            params.rainIntensity = 0.0f;
            break;
            
        case CLOUDY:
            params.cloudCoverage = 0.8f;
            params.cloudDensity = 0.9f;
            params.rainIntensity = 0.0f;
            break;
            
        case LIGHT_RAIN:
            params.cloudCoverage = 0.9f;
            params.cloudDensity = 0.9f;
            params.rainIntensity = 0.4f;
            break;
            
        case HEAVY_RAIN:
            params.cloudCoverage = 1.0f;
            params.cloudDensity = 1.0f;
            params.rainIntensity = 1.0f;
            break;
    }
    
    return params;
}

void WeatherController::UpdateTransition(float deltaTime) {
    // Update the transition timer
    transitionTimer += deltaTime;
    float t = glm::min(transitionTimer / transitionDuration, 1.0f);
    
    // Get parameters for current and target states
    WeatherParams currentParams = GetParamsForState(currentState);
    WeatherParams targetParams = GetParamsForState(targetState);
    
    // Interpolate parameters
    float cloudCoverage = glm::mix(currentParams.cloudCoverage, targetParams.cloudCoverage, t);
    float cloudDensity = glm::mix(currentParams.cloudDensity, targetParams.cloudDensity, t);
    float rainIntensity = glm::mix(currentParams.rainIntensity, targetParams.rainIntensity, t);
    
    // Apply to systems
    cloudSystem->cloudCoverage = cloudCoverage;
    cloudSystem->cloudDensity = cloudDensity;
    rainSystem->rainIntensity = rainIntensity;
    
    // Check if transition is complete
    if (t >= 1.0f) {
        currentState = targetState;
    }
}