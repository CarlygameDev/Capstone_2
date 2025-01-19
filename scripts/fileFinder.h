#pragma once
#include <iostream>
#include <filesystem>
class fileFinder {
public:
	static std::string getTexture(const std::string& path) {
		std::filesystem::path cwd = std::filesystem::current_path();
		cwd.append("textures").append(path);
	//	std::cout << cwd.string()<<std::endl;
		return cwd.string();
	}
	static std::string getPath(const std::string& path) {
		std::filesystem::path cwd = std::filesystem::current_path();
		cwd.append(path);
		//	std::cout << cwd.string()<<std::endl;
		return cwd.string();
	}
	static std::string getShaderPath(const std::string& path) {
		std::filesystem::path cwd = std::filesystem::current_path();
		cwd.append("shaders").append(path);
		//	std::cout << cwd.string()<<std::endl;
		return cwd.string();
	}
	static std::string getModelPath(const std::string& path) {
		std::filesystem::path cwd = std::filesystem::current_path();
		cwd.append("models").append(path);
		//	std::cout << cwd.string()<<std::endl;
		return cwd.string();
	}
};