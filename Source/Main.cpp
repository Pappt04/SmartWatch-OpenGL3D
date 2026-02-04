#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <algorithm>

#include "../Header/Util.h"
#include "../Header/Camera.h"
#include "../Header/HandController.h"
#include "../Header/Models.h"
#include "../Header/RunningSimulation.h"
#include "../Header/DigitRenderer.h"

// Global variables
int wWidth = 1200, wHeight = 800;
Camera* g_camera = nullptr;
HandController* g_handController = nullptr;
RunningSimulation* g_runningSimulation = nullptr;
DigitRenderer* g_digitRenderer = nullptr;

double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;
bool g_firstMouse = true;
bool g_isRunning = false;
bool g_freeCameraMode = false;

// Screen state
enum ScreenType {
    SCREEN_CLOCK,
    SCREEN_HEART_RATE,
    SCREEN_BATTERY
};
ScreenType g_currentScreen = SCREEN_CLOCK;

// Simulation State
int g_heartRate = 70;
double g_lastHeartUpdate = 0.0;
int g_batteryPercent = 100;
double g_lastBatteryUpdate = 0.0;
float g_ecgScrollOffset = 0.0f; // ECG texture scroll position

// Time Simulation
int g_hours = 12, g_minutes = 30, g_seconds = 0;
double g_lastTimeUpdate = 0.0;

// Resources
unsigned int g_roadTexture = 0;
unsigned int g_warningTexture = 0;
unsigned int g_ecgTexture = 0;
unsigned int g_batteryTexture = 0;
unsigned int g_arrowRightTexture = 0;
unsigned int g_armTexture = 0;
unsigned int g_sunTexture = 0;

GLFWcursor* g_heartCursor = nullptr;

// Arrow screen positions for click detection
glm::vec2 g_leftArrowScreenPos(0.0f);
glm::vec2 g_rightArrowScreenPos(0.0f);
float g_arrowClickRadius = 50.0f; // Pixels

// Helper function to project 3D point to screen space
glm::vec2 projectToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& proj) {
    glm::vec4 clipPos = proj * view * glm::vec4(worldPos, 1.0f);
    if (clipPos.w <= 0.0f) return glm::vec2(-1000.0f); // Behind camera
    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
    float screenX = (ndc.x + 1.0f) * 0.5f * wWidth;
    float screenY = (1.0f - ndc.y) * 0.5f * wHeight; // Flip Y
    return glm::vec2(screenX, screenY);
}

// Mouse callback - handle camera movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // When viewing watch and not in free camera mode, don't process mouse movement
    if (g_handController && g_handController->isInViewingMode() && !g_freeCameraMode) {
        return;
    }

    if (g_firstMouse) {
        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
        g_firstMouse = false;
        return;
    }

    double xoffset = xpos - g_lastMouseX;
    double yoffset = g_lastMouseY - ypos; // Reversed since y-coords go from bottom to top
    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    if (g_camera) {
        if (g_freeCameraMode) {
            // Free camera mode - rotate in any direction
            g_camera->rotate((float)xoffset, (float)yoffset);
        } else {
            // Normal mode - only vertical movement
            g_camera->moveVertical((float)yoffset * 0.005f);
        }
    }
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (!g_handController || !g_handController->isInViewingMode()) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        glm::vec2 mousePos((float)xpos, (float)ypos);

        // Check distance to left arrow
        float distToLeft = glm::length(mousePos - g_leftArrowScreenPos);
        if (distToLeft < g_arrowClickRadius) {
            if (g_currentScreen == SCREEN_HEART_RATE) g_currentScreen = SCREEN_CLOCK;
            else if (g_currentScreen == SCREEN_BATTERY) g_currentScreen = SCREEN_HEART_RATE;
            return;
        }

        // Check distance to right arrow
        float distToRight = glm::length(mousePos - g_rightArrowScreenPos);
        if (distToRight < g_arrowClickRadius) {
            if (g_currentScreen == SCREEN_CLOCK) g_currentScreen = SCREEN_HEART_RATE;
            else if (g_currentScreen == SCREEN_HEART_RATE) g_currentScreen = SCREEN_BATTERY;
            return;
        }
    }
}

// Key callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (g_handController) {
            g_handController->toggleState();
            // Reset mouse tracking to prevent camera jump
            g_firstMouse = true;
        }
    }

    if (key == GLFW_KEY_K && action == GLFW_PRESS) {
        g_freeCameraMode = !g_freeCameraMode;
        g_firstMouse = true; // Reset mouse tracking to prevent jump
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

void renderQuad(unsigned int shader, unsigned int texture, float x, float y, float w, float h, const glm::mat4& parentModel, bool flipX = false) {
    glm::mat4 model = parentModel;
    model = glm::translate(model, glm::vec3(x, y, 0.01f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    if (flipX) model = glm::scale(model, glm::vec3(-1.0f, 1.0f, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader, "uM"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shader, "uUseTexture"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shader, "uTexture"), 0);

    // Lower illumination for watch screen content
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.3f)));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.4f)));
    
    static unsigned int quadVAO = 0;
    if (quadVAO == 0) {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
        };
        unsigned int quadVBO;
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// Render ECG with animated texture coordinates based on heart rate
void renderECG(unsigned int shader, unsigned int texture, float x, float y, float w, float h,
               const glm::mat4& parentModel, float scrollOffset, float texScale, float heightScale) {
    glm::mat4 model = parentModel;
    model = glm::translate(model, glm::vec3(x, y, 0.01f));
    model = glm::scale(model, glm::vec3(w, h * heightScale, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader, "uM"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shader, "uUseTexture"), 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shader, "uTexture"), 0);

    // Green ECG line color
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.0f, 0.8f, 0.0f)));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.0f, 0.5f, 0.0f)));

    // Create dynamic vertices with scrolling texture coordinates
    float u0 = scrollOffset;
    float u1 = scrollOffset + texScale;

    float vertices[] = {
        -0.5f, -0.5f, 0.0f, u0, 0.0f,
         0.5f, -0.5f, 0.0f, u1, 0.0f,
         0.5f,  0.5f, 0.0f, u1, 1.0f,
        -0.5f, -0.5f, 0.0f, u0, 0.0f,
         0.5f,  0.5f, 0.0f, u1, 1.0f,
        -0.5f,  0.5f, 0.0f, u0, 1.0f
    };

    static unsigned int ecgVAO = 0, ecgVBO = 0;
    if (ecgVAO == 0) {
        glGenVertexArrays(1, &ecgVAO);
        glGenBuffers(1, &ecgVBO);
        glBindVertexArray(ecgVAO);
        glBindBuffer(GL_ARRAY_BUFFER, ecgVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
    } else {
        glBindVertexArray(ecgVAO);
        glBindBuffer(GL_ARRAY_BUFFER, ecgVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    }

    glBindVertexArray(ecgVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void renderScreenContent(unsigned int shader, ScreenType screen, double currentTime, const glm::mat4& parentModel, float contentScale = 1.0f) {
    // Draw white screen background first - sized to fit watch screen (0.25f geometry)
    glm::mat4 bgModel = parentModel;
    bgModel = glm::translate(bgModel, glm::vec3(0.0f, 0.0f, -0.001f)); // Slightly behind content
    bgModel = glm::scale(bgModel, glm::vec3(0.23f, 0.23f, 1.0f)); // Fixed size to fit screen
    glUniformMatrix4fv(glGetUniformLocation(shader, "uM"), 1, GL_FALSE, glm::value_ptr(bgModel));
    glUniform1i(glGetUniformLocation(shader, "uUseTexture"), 0);
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.9f, 0.9f, 0.9f))); // White
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.8f, 0.8f, 0.8f)));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.1f)));
    glUniform1f(glGetUniformLocation(shader, "uMaterial.shine"), 4.0f);

    // Draw circular background
    static unsigned int bgVAO = 0;
    static int bgVertexCount = 0;
    if (bgVAO == 0) {
        const int segments = 32;
        std::vector<float> vertices;
        // Center vertex
        vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
        // Edge vertices
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / (float)segments * 2.0f * 3.14159265f;
            vertices.push_back(0.5f * cos(angle));
            vertices.push_back(0.5f * sin(angle));
            vertices.push_back(0.0f);
        }
        // Create triangle fan indices as vertex data
        std::vector<float> fanVertices;
        for (int i = 1; i <= segments; i++) {
            // Center
            fanVertices.push_back(vertices[0]); fanVertices.push_back(vertices[1]); fanVertices.push_back(vertices[2]);
            // Current edge
            fanVertices.push_back(vertices[i*3]); fanVertices.push_back(vertices[i*3+1]); fanVertices.push_back(vertices[i*3+2]);
            // Next edge
            fanVertices.push_back(vertices[(i+1)*3]); fanVertices.push_back(vertices[(i+1)*3+1]); fanVertices.push_back(vertices[(i+1)*3+2]);
        }
        bgVertexCount = segments * 3;
        unsigned int bgVBO;
        glGenVertexArrays(1, &bgVAO);
        glGenBuffers(1, &bgVBO);
        glBindVertexArray(bgVAO);
        glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
        glBufferData(GL_ARRAY_BUFFER, fanVertices.size() * sizeof(float), fanVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
    glBindVertexArray(bgVAO);
    glDrawArrays(GL_TRIANGLES, 0, bgVertexCount);
    glBindVertexArray(0);

    // Scale all positions and sizes by contentScale
    float s = contentScale;

    // Draw arrows - smaller and inside the watch screen
    if (screen != SCREEN_CLOCK) {
        renderQuad(shader, g_arrowRightTexture, -0.14f * s, 0.0f, 0.04f * s, 0.04f * s, parentModel, true);
    }
    if (screen != SCREEN_BATTERY) {
        renderQuad(shader, g_arrowRightTexture, 0.14f * s, 0.0f, 0.04f * s, 0.04f * s, parentModel, false);
    }

    if (screen == SCREEN_CLOCK) {
        float scale = 0.045f * s;
        float totalWidth = (6 * 0.6f + 2 * 0.3f) * scale;
        // Dark text on white background
        g_digitRenderer->drawTime(g_hours, g_minutes, g_seconds, -totalWidth/2.0f, -0.02f * s, scale, glm::vec3(0.1f, 0.1f, 0.1f), shader, parentModel);
    }
    else if (screen == SCREEN_HEART_RATE) {
        // ECG with dynamic scrolling and scaling based on heart rate
        // texScale: more beats visible at higher heart rate (texture repeats more)
        float texScale = 2.0f + ((g_heartRate - 60.0f) / 150.0f) * 2.0f;
        texScale = std::max(1.5f, std::min(4.0f, texScale));

        // heightScale: larger amplitude at higher heart rate
        float heightScale = 1.0f + ((g_heartRate - 70.0f) / 150.0f) * 0.5f;
        heightScale = std::max(1.0f, std::min(1.5f, heightScale));

        // Render ECG with animated texture
        renderECG(shader, g_ecgTexture, 0.0f, -0.05f * s, 0.22f * s, 0.08f * s,
                  parentModel, g_ecgScrollOffset, texScale, heightScale);

        float scale = 0.035f * s;
        // Red BPM text
        g_digitRenderer->drawNumber(g_heartRate, -0.06f * s, 0.08f * s, scale, glm::vec3(0.8f, 0.0f, 0.0f), shader, parentModel);

        if (g_heartRate > 200) {
            renderQuad(shader, g_warningTexture, 0.0f, 0.0f, 0.3f * s, 0.3f * s, parentModel);
        }
    }
    else if (screen == SCREEN_BATTERY) {
        renderQuad(shader, g_batteryTexture, 0.0f, 0.0f, 0.16f * s, 0.09f * s, parentModel);

        float barWidth = 0.13f * s * (g_batteryPercent / 100.0f);

        glm::vec3 barColor(0.0f, 0.8f, 0.0f);
        if (g_batteryPercent < 20) barColor = glm::vec3(0.9f, 0.8f, 0.0f);
        if (g_batteryPercent < 10) barColor = glm::vec3(0.9f, 0.0f, 0.0f);

        glm::mat4 model = parentModel;
        model = glm::translate(model, glm::vec3(-0.065f * s + barWidth/2.0f, 0.0f, 0.02f));
        model = glm::scale(model, glm::vec3(barWidth, 0.065f * s, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shader, "uM"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(glGetUniformLocation(shader, "uUseTexture"), 0);
        glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(barColor));
        glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(barColor));

        static unsigned int rectVAO = 0;
        if (rectVAO == 0) {
            float vertices[] = {
                -0.5f, -0.5f, 0.0f,
                 0.5f, -0.5f, 0.0f,
                 0.5f,  0.5f, 0.0f,
                -0.5f, -0.5f, 0.0f,
                 0.5f,  0.5f, 0.0f,
                -0.5f,  0.5f, 0.0f
            };
            unsigned int rectVBO;
            glGenVertexArrays(1, &rectVAO);
            glGenBuffers(1, &rectVBO);
            glBindVertexArray(rectVAO);
            glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        float scale = 0.035f * s;
        // Dark text on white background
        g_digitRenderer->drawNumber(g_batteryPercent, -0.025f * s, 0.07f * s, scale, glm::vec3(0.1f, 0.1f, 0.1f), shader, parentModel);
        g_digitRenderer->drawPercent(0.04f * s, 0.07f * s, scale, glm::vec3(0.1f, 0.1f, 0.1f), shader, parentModel);
    }
}

int main() {
    if (!glfwInit()) return endProgram("GLFW failed to initialize");
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(wWidth, wHeight, "SmartWatch3D", NULL, NULL);
    if (!window) return endProgram("Window creation failed");
    glfwMakeContextCurrent(window);
    
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    // Cursor always visible
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    if (glewInit() != GLEW_OK) return endProgram("GLEW initialization failed");
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Sky blue gradient effect for outdoor atmosphere
    glClearColor(0.45f, 0.55f, 0.7f, 1.0f);
    
    unsigned int phongShader = createShader("phong.vert", "phong.frag");

    g_roadTexture = loadImageToTexture("Resources/road.jpg");
    g_warningTexture = loadImageToTexture("Resources/textures/warning.png");
    g_ecgTexture = loadImageToTexture("Resources/textures/ecg_wave.png");
    g_batteryTexture = loadImageToTexture("Resources/textures/battery.png");
    g_arrowRightTexture = loadImageToTexture("Resources/textures/arrow_right.png");
    g_armTexture = loadImageToTexture("Resources/arm/arm.jpg");
    g_sunTexture = loadImageToTexture("Resources/sun/2k_sun.jpg");
    g_heartCursor = loadImageToCursor("Resources/textures/red_heart_cursor.png");

    // Set heart cursor always
    if (g_heartCursor) glfwSetCursor(window, g_heartCursor);
    
    // Camera positioned low in the street, close to hand level
    g_camera = new Camera(glm::vec3(0.0f, 1.4f, 0.0f), (float)wWidth / (float)wHeight);
    g_handController = new HandController();
    // Larger segments to match wider road, more segments for extended scene
    g_runningSimulation = new RunningSimulation(15.0f, 12); // More road segments for longer view
    g_digitRenderer = new DigitRenderer();
    g_digitRenderer->init();

    // Large ground plane extending to horizon with fog fade
    Mesh groundPlane = Geometry::createGroundPlane(200.0f, 400.0f, 50);
    // Load arm model from OBJ file
    Model* armModel = new Model("Resources/arm/arm.obj");
    // Load sun model from OBJ file
    Model* sunModel = new Model("Resources/sun/sol.obj");
    // Circular watch screen for display
    Mesh watchScreen = Geometry::createWatchScreen(0.25f, 32);
    // Cylindrical watch body/case around the screen
    Mesh watchBody = Geometry::createWatchBody(0.3f, 0.04f, 32);
    // Wider road for better perspective
    Mesh roadSegment = Geometry::createRoadSegment(8.0f, 15.0f);
    
    std::vector<Model*> buildings;
    buildings.push_back(new Model("Resources/ChonkyBuilding/chonky_buildingA.obj"));
    buildings.push_back(new Model("Resources/Skyscraper/skyscraperE.obj"));
    buildings.push_back(new Model("Resources/TallBuilding/tall_buildingC.obj"));
    buildings.push_back(new Model("Resources/Large Building/large_buildingE.obj"));

    // Dense building placement along both sides of the road
    // Buildings are larger (scale 3.0-5.0) so they tower over the camera
    // Building models loaded above
    // Building placement managed by RunningSimulation

    // Main sky/sun light - positioned in front of camera, visible in sky
    glm::vec3 lightPos(-30.0f, 80.0f, -150.0f); // High in the sky, in front of camera
    glm::vec3 lightAmbient(0.25f, 0.25f, 0.28f); // Slightly blue ambient for outdoor feel
    glm::vec3 lightDiffuse(1.0f, 0.95f, 0.85f); // Warm sunlight
    glm::vec3 lightSpecular(1.0f, 1.0f, 0.9f);

    // Watch screen light parameters (very weak secondary light)
    glm::vec3 watchLightAmbient(0.005f, 0.005f, 0.008f);
    glm::vec3 watchLightDiffuse(0.03f, 0.04f, 0.05f); // Very dim screen glow
    glm::vec3 watchLightSpecular(0.02f, 0.02f, 0.03f);
    
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Free camera movement (WASD + Q/E for up/down)
        if (g_freeCameraMode && g_camera) {
            float cameraSpeed = 5.0f * (float)deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                g_camera->moveForward(cameraSpeed);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                g_camera->moveForward(-cameraSpeed);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                g_camera->moveRight(-cameraSpeed);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                g_camera->moveRight(cameraSpeed);
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
                g_camera->moveUp(-cameraSpeed);
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
                g_camera->moveUp(cameraSpeed);
        }

        g_isRunning = !g_freeCameraMode &&
                      (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) &&
                      (g_currentScreen == SCREEN_HEART_RATE) &&
                      (g_handController->isInViewingMode());

        // Update camera bobbing first, then get position for hand
        g_camera->updateBobbing(deltaTime, g_isRunning);
        glm::vec3 camPos = g_camera->getPosition();

        // Hand follows camera
        g_handController->update(deltaTime, camPos);
        g_runningSimulation->update(deltaTime, g_isRunning);
        
        if (currentTime - g_lastTimeUpdate >= 1.0) {
            g_lastTimeUpdate = currentTime;
            g_seconds++;
            if (g_seconds >= 60) { g_seconds = 0; g_minutes++; }
            if (g_minutes >= 60) { g_minutes = 0; g_hours++; }
            if (g_hours >= 24) g_hours = 0;
        }
        
        srand(currentTime);
        if (currentTime - g_lastHeartUpdate >= (g_isRunning ? 0.05 : 0.1)) {
            g_lastHeartUpdate = currentTime;
            if (g_isRunning) {
                if (g_heartRate < 220) g_heartRate++;
            } else {
                if (g_heartRate > 70) g_heartRate--;
                else if (g_heartRate < 60) g_heartRate++;
                else g_heartRate += (rand() % 3 - 1);
            }
        }

        // Update ECG scroll offset - speed increases with heart rate
        float ecgSpeed = 0.3f * (g_heartRate / 70.0f);
        g_ecgScrollOffset += ecgSpeed * (float)deltaTime;
        if (g_ecgScrollOffset > 100.0f) g_ecgScrollOffset -= 100.0f; // Wrap around

        if (currentTime - g_lastBatteryUpdate >= 10.0) {
            g_lastBatteryUpdate = currentTime;
            if (g_batteryPercent > 0) g_batteryPercent--;
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glm::mat4 view = g_camera->getViewMatrix();
        glm::mat4 projection = g_camera->getProjectionMatrix();
        glm::vec3 cameraPos = g_camera->getPosition();
        
        glUseProgram(phongShader);
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(phongShader, "uViewPos"), 1, glm::value_ptr(cameraPos));
        
        glUniform3fv(glGetUniformLocation(phongShader, "uLight.pos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(phongShader, "uLight.kA"), 1, glm::value_ptr(lightAmbient));
        glUniform3fv(glGetUniformLocation(phongShader, "uLight.kD"), 1, glm::value_ptr(lightDiffuse));
        glUniform3fv(glGetUniformLocation(phongShader, "uLight.kS"), 1, glm::value_ptr(lightSpecular));

        // Watch screen light position (updated dynamically based on watch position)
        glm::mat4 handM_temp = g_handController->getTransformMatrix();
        glm::vec3 watchScreenPos = glm::vec3(handM_temp * glm::vec4(0.0f, 0.1f, -0.02f, 1.0f));
        glUniform3fv(glGetUniformLocation(phongShader, "uWatchLight.pos"), 1, glm::value_ptr(watchScreenPos));
        glUniform3fv(glGetUniformLocation(phongShader, "uWatchLight.kA"), 1, glm::value_ptr(watchLightAmbient));
        glUniform3fv(glGetUniformLocation(phongShader, "uWatchLight.kD"), 1, glm::value_ptr(watchLightDiffuse));
        glUniform3fv(glGetUniformLocation(phongShader, "uWatchLight.kS"), 1, glm::value_ptr(watchLightSpecular));
        glUniform1i(glGetUniformLocation(phongShader, "uUseWatchLight"), g_handController->isInViewingMode() ? 1 : 0);

        // Set fog parameters for natural horizon fade
        glUniform1i(glGetUniformLocation(phongShader, "uUseFog"), 1);
        glUniform3fv(glGetUniformLocation(phongShader, "uFogColor"), 1, glm::value_ptr(glm::vec3(0.45f, 0.55f, 0.7f))); // Match sky color
        glUniform1f(glGetUniformLocation(phongShader, "uFogDensity"), 0.00025f);

        // Render Ground - solid green color (no texture)
        glm::mat4 groundModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(groundModel));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.2f, 0.6f, 0.15f))); // Green grass color
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.1f, 0.25f, 0.08f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.02f, 0.02f, 0.02f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 2.0f);
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 0);
        groundPlane.draw();

        // Render Sun at the light source position (no fog for sun)
        glUniform1i(glGetUniformLocation(phongShader, "uUseFog"), 0);
        glm::mat4 sunTransform = glm::mat4(1.0f);
        sunTransform = glm::translate(sunTransform, lightPos); // Position at light source
        sunTransform = glm::scale(sunTransform, glm::vec3(25.0f)); // Scale sun to be visible in sky
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(sunTransform));
        // Bright emissive sun material
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(1.0f, 0.9f, 0.7f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(1.0f, 0.95f, 0.8f))); // High ambient for glow
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.0f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 1.0f);
        // Use sun texture
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_sunTexture);
        glUniform1i(glGetUniformLocation(phongShader, "uTexture"), 0);
        sunModel->draw();
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 0);
        glUniform1i(glGetUniformLocation(phongShader, "uUseFog"), 1); // Re-enable fog for other objects

        // Render Road Segments - with road texture
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.6f, 0.6f, 0.6f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.25f, 0.25f, 0.25f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.1f, 0.1f, 0.1f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 4.0f);
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_roadTexture);
        glUniform1i(glGetUniformLocation(phongShader, "uTexture"), 0);
        for (float zPos : g_runningSimulation->getSegmentPositions()) {
            glm::mat4 segmentModel = glm::mat4(1.0f);
            segmentModel = glm::translate(segmentModel, glm::vec3(0.0f, 0.01f, zPos));
            glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(segmentModel));
            roadSegment.draw();
        }
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 0);
        
        // Render Buildings - dense placement with varying scales
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.6f, 0.55f, 0.5f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.4f, 0.38f, 0.35f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.2f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 16.0f);

        const auto& simBuildings = g_runningSimulation->getBuildings();
        for (const auto& b : simBuildings) {
            glm::mat4 bModel = glm::mat4(1.0f);
            bModel = glm::translate(bModel, b.position);
            bModel = glm::scale(bModel, glm::vec3(b.scale));
            
            glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(bModel));
            buildings[b.type]->draw();
        }
        
        // Disable fog for hand and watch (they're close to camera)
        glUniform1i(glGetUniformLocation(phongShader, "uUseFog"), 0);

        // Render Arm - with skin-like color
        glm::mat4 handM = g_handController->getTransformMatrix();
        // Transform arm model - rotate so elbow is at bottom, move back
        glm::mat4 armTransform = handM;
        armTransform = glm::translate(armTransform, glm::vec3(0.0f, 0.0f, 0.3f)); // Move back (less in front)
        armTransform = glm::rotate(armTransform, glm::radians(180.0f), glm::vec3(0.0f, 0.1f, 1.0f)); // Rotate so elbow is at bottom
        armTransform = glm::scale(armTransform, glm::vec3(0.02f)); // Scale to fit scene

        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(armTransform));
        // Skin-like material colors
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.85f, 0.72f, 0.62f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.4f, 0.32f, 0.28f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.25f, 0.22f, 0.2f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 12.0f);
        // Render with skin color (no texture for now)
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 0);
        armModel->draw();

        // Watch position - at the wrist area (same as original cube)
        glm::mat4 watchM = handM;
        watchM = glm::translate(watchM, glm::vec3(-0.15f, 0.0f, -0.05f)); // On top of wrist, at start of hand
        // Rotate -90 degrees around X axis so screen faces UP (toward camera), parallel to arm length
        watchM = glm::rotate(watchM, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Render black watch body/case
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(watchM));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.02f, 0.02f, 0.02f))); // Very dark black
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.01f, 0.01f, 0.01f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.1f, 0.1f, 0.1f))); // Less shiny
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 16.0f);
        watchBody.draw();

        // Watch screen (slightly in front of body toward camera)
        glm::mat4 screenM = watchM;
        screenM = glm::translate(screenM, glm::vec3(0.0f, 0.0f, 0.021f)); // Front of watch body

        // Scale content for watch screen - sized to fit within watch body
        float contentScale = 0.55f;

        // Calculate arrow screen positions for click detection
        glm::vec3 leftArrowWorld = glm::vec3(screenM * glm::vec4(-0.14f * contentScale, 0.0f, 0.01f, 1.0f));
        glm::vec3 rightArrowWorld = glm::vec3(screenM * glm::vec4(0.14f * contentScale, 0.0f, 0.01f, 1.0f));
        g_leftArrowScreenPos = projectToScreen(leftArrowWorld, view, projection);
        g_rightArrowScreenPos = projectToScreen(rightArrowWorld, view, projection);

        // Render screen content
        renderScreenContent(phongShader, g_currentScreen, currentTime, screenM, contentScale);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    groundPlane.cleanup();
    delete armModel;
    delete sunModel;
    watchScreen.cleanup();
    watchBody.cleanup();
    roadSegment.cleanup();
    
    for (auto* building : buildings) delete building;
    
    delete g_camera;
    delete g_handController;
    delete g_runningSimulation;
    delete g_digitRenderer;
    
    glDeleteProgram(phongShader);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}