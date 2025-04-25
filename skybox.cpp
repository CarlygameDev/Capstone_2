
#pragma once
#include <vector>
#include <string>

unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath);
unsigned int loadCubemap(std::vector<std::string> faces);
std::string loadShaderSource(const char* filepath);
