#pragma once
#include "cloudSystem.h"
#include "rainSystem.h"
#include <glm/glm.hpp>

enum WeatherState {
    CLEAR,
    PARTLY_CLOUDY,
    CLOUDY,
    LIGHT_RAIN,
    HEAVY_RAIN
};

class WeatherController {
public:
    WeatherController(CloudSystem* cloudSys, RainSystem* rainSys);
    
    void Update(float deltaTime);
    void SetWeatherState(WeatherState state);
    WeatherState GetCurrentWeatherState() const { return currentState; }
    void CycleWeatherState();
    
    // Wind control
    void SetWindDirection(const glm::vec3& direction);
    glm::vec3 GetWindDirection() const { return windDirection; }
    
private:
    CloudSystem* cloudSystem;
    RainSystem* rainSystem;
    
    WeatherState currentState;
    WeatherState targetState;
    
    float transitionTimer;
    float transitionDuration;
    
    glm::vec3 windDirection;
    
    // Target values for different weather states
    struct WeatherParams {
        float cloudCoverage;
        float cloudDensity;
        float rainIntensity;
    };
    
    WeatherParams GetParamsForState(WeatherState state);
    void UpdateTransition(float deltaTime);
};