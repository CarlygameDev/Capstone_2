#include <filesystem>
#include <glad/glad.h>
#include "camera.h"
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <cmath>
#define M_PI 3.14159265358979323846


// Camera setup
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 400, lastY = 300;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

// Weather toggles
bool showWeather = false;
bool thunder = false;
float thunderTimer = 0;

// Structs
struct Cloud { float x, y; };
struct Raindrop { float x, y; };
std::vector<Cloud> clouds;
std::vector<Raindrop> raindrops;

// Shader/Texture helpers...
std::string loadShaderSource(const char* path) {
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

unsigned int createShaderProgram(const char* vert, const char* frag) {
    std::string vCode = loadShaderSource(vert);
    std::string fCode = loadShaderSource(frag);
    const char* vSrc = vCode.c_str();
    const char* fSrc = fCode.c_str();
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vSrc, nullptr); glCompileShader(vs);
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fSrc, nullptr); glCompileShader(fs);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

unsigned int loadCubemap(const std::vector<std::string>& faces) {
    unsigned int id; glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    int w, h, channels;
    for (unsigned int i = 0; i < faces.size(); ++i) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &channels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cerr << "Failed to load cubemap texture at: " << faces[i] << "\n";
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return id;
}

void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    camera.ProcessMouseMovement(xOffset, yOffset);
}

void processInput(GLFWwindow* window) {
    float current = glfwGetTime();
    deltaTime = current - lastFrame;
    lastFrame = current;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

    static bool keyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !keyPressed) {
        showWeather = !showWeather;
        keyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        keyPressed = false;
    }
}

void drawRain() {
    glBegin(GL_LINES);
    glColor3f(0.7f, 0.7f, 1.0f);
    for (auto& r : raindrops) {
        glVertex2f(r.x, r.y); glVertex2f(r.x, r.y - 10);
        r.y -= 5; if (r.y < -300) { r.y = 300; r.x = rand() % 800 - 400; }
    }
    glEnd();
}

void drawClouds() {
    glColor3f(0.9f, 0.9f, 0.9f);
    for (auto& c : clouds) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(c.x, c.y);
        for (int i = 0; i <= 20; i++) {
            float a = 2 * M_PI * i / 20;
            glVertex2f(c.x + cos(a) * 50, c.y + sin(a) * 30);
        }
        glEnd();
        c.x += 0.1f; if (c.x > 400) c.x = -400;
    }
}

void drawThunder() {
    if (!thunder) return;
    glColor3f(1, 1, 0.8);
    glBegin(GL_TRIANGLES);
    glVertex2f(50, 150); glVertex2f(70, 100); glVertex2f(40, 120);
    glEnd();
    thunderTimer -= deltaTime;
    if (thunderTimer <= 0) thunder = false;
}

int main() {
    srand((unsigned)time(0));
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Skybox Cube", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST);
    std::cout << "Working Directory: " << std::filesystem::current_path() << "\n";

    std::vector<std::string> faces = {
        "textures/right1.jpg", "textures/left1.jpg", "textures/top1.jpg",
        "textures/bottom1.jpg", "textures/front1.jpg", "textures/back1.jpg"
    };

    float skyboxVertices[] = {
        // (same cube layout as before...)
        -1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,-1,
        -1,-1,1,-1,-1,-1,-1,1,-1,-1,1,-1,-1,1,1,-1,-1,1,
        1,-1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,
        -1,-1,1,-1,1,1,1,1,1,1,1,1,1,-1,1,-1,-1,1,
        -1,1,-1,1,1,-1,1,1,1,1,1,1,-1,1,1,-1,1,-1,
        -1,-1,-1,-1,-1,1,1,-1,-1,1,-1,-1,-1,-1,1,1,-1,1
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    unsigned int cubemap = loadCubemap(faces);
    unsigned int shader = createShaderProgram("skybox.vert", "skybox.frag");
    glUseProgram(shader); glUniform1i(glGetUniformLocation(shader, "skybox"), 0);

    for (int i = 0; i < 50; i++) clouds.push_back({ (float)(rand() % 800 - 400), (float)(rand() % 200 + 200) });
    for (int i = 0; i < 100; i++) raindrops.push_back({ (float)(rand() % 800 - 400), (float)(rand() % 600) });

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL);
        glUseProgram(shader);

        glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 100.f);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        if (showWeather) {
            glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(-400, 400, -300, 300, -1, 1);
            glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            drawClouds(); drawRain();

            if (rand() % 100 < 1 && !thunder) {
                thunder = true;
                thunderTimer = 0.5f;
            }
            drawThunder();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}



