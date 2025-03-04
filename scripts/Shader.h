#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fileFinder.h>

class ShaderBase
{
public:
    unsigned int ID;

    // Constructor (empty, to be called by derived classes)
    // Activate the shader
    void use() const {
        glUseProgram(ID);
    }

    // utility uniform functions
   // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string& name, float x, float y, float z, float w)
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }


};

class Shader : public ShaderBase
{
public:
   
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr, const char* TControlPath = nullptr, const char* TEvaluationPath = nullptr)
    {
        std::string vertexCode, fragmentCode, geometryCode, tControlCode, tEvaluationCode;

        try {
            // Load the main vertex and fragment shader code
            vertexCode = loadShaderCode(vertexPath);
            fragmentCode = loadShaderCode(fragmentPath);

            if (geometryPath != nullptr) geometryCode = loadShaderCode(geometryPath);

            if (TEvaluationPath != nullptr) {
                Tesselation = true;
                tEvaluationCode = loadShaderCode(TEvaluationPath);
                // Only load the TCS if the TES is present
                if (TControlPath != nullptr) {
                    tControlCode = loadShaderCode(TControlPath);
                }
            }
        }
        catch (std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }

        // Compile the shaders
        unsigned int vertex = compileShader(vertexCode.c_str(), GL_VERTEX_SHADER, vertexPath);
        unsigned int fragment = compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER, fragmentPath);

        unsigned int geometry = 0, tControl = 0, tEvaluation = 0;
        if (geometryPath != nullptr) geometry = compileShader(geometryCode.c_str(), GL_GEOMETRY_SHADER, geometryPath);
        if (TEvaluationPath != nullptr) tEvaluation = compileShader(tEvaluationCode.c_str(), GL_TESS_EVALUATION_SHADER, TEvaluationPath);
        if (tControlCode != "") tControl = compileShader(tControlCode.c_str(), GL_TESS_CONTROL_SHADER, TControlPath);

        // Link the shaders into the program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (geometry) glAttachShader(ID, geometry);
        if (tControl) glAttachShader(ID, tControl);
        if (tEvaluation) glAttachShader(ID, tEvaluation);

        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM", fragmentPath);

        // Clean up shaders after linking
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometry) glDeleteShader(geometry);
        if (tControl) glDeleteShader(tControl);
        if (tEvaluation) glDeleteShader(tEvaluation);
    }


    const bool hasTesselation() {
        return Tesselation;
    }
    
private:
    bool Tesselation=false;
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type, const std::string& name)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type
                    << " in file: " << name << "\n" << infoLog
                    << "\n -- --------------------------------------------------- -- "
                    << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type
                    << " in file: " << name << "\n" << infoLog
                    << "\n -- --------------------------------------------------- -- "
                    << std::endl;
            }
        }
    }
    std::string loadShaderCode(const char* path) {
        std::ifstream shaderFile(fileFinder::getShaderPath(path));
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        return shaderStream.str();
    }

    const char* getShaderTypeName(GLenum shaderType) {
        switch (shaderType) {
        case GL_VERTEX_SHADER: return "VERTEX";
        case GL_FRAGMENT_SHADER: return "FRAGMENT";
        case GL_GEOMETRY_SHADER: return "GEOMETRY";
        case GL_TESS_CONTROL_SHADER: return "TESS_CONTROL";
        case GL_TESS_EVALUATION_SHADER: return "TESS_EVALUATION";
        default: return "UNKNOWN";
        }
    }
    unsigned int compileShader(const char* shaderCode, GLenum shaderType, const char* shaderPath) {
        unsigned int shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &shaderCode, NULL);
        glCompileShader(shader);
        checkCompileErrors(shader, getShaderTypeName(shaderType), shaderPath);
        return shader;
    }
};

class ComputeShader : public ShaderBase
{
public:
   
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------

    ComputeShader(const char* computePath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string computeCode;
        std::ifstream cShaderFile;
        // ensure ifstream objects can throw exceptions:
        cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            cShaderFile.open(fileFinder::getShaderPath( computePath));

            std::stringstream cShaderStream;
            // read file's buffer contents into streams
            cShaderStream << cShaderFile.rdbuf();
            // close file handlers
            cShaderFile.close();
            // convert stream into string
            computeCode = cShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        const char* cShaderCode = computeCode.c_str();
        // 2. compile shaders
        unsigned int compute;
        // compute shader
        compute = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compute, 1, &cShaderCode, NULL);
        glCompileShader(compute);
        checkCompileErrors(compute, computePath);

        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, compute);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(compute);
    }


private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

#endif