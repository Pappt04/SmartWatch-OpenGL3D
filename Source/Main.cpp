#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "../Header/Util.h"
#include "../Header/Camera.h"
#include "../Header/HandController.h"
#include "../Header/Models.h"
#include "../Header/RunningSimulation.h"

// Global variables
int wWidth = 1200, wHeight = 800;
Camera* g_camera = nullptr;
HandController* g_handController = nullptr;
RunningSimulation* g_runningSimulation = nullptr;

double g_lastMouseY = 0.0;
bool g_firstMouse = true;
bool g_isRunning = false;

// Screen state
enum ScreenType {
    SCREEN_CLOCK,
    SCREEN_HEART_RATE,
    SCREEN_BATTERY
};
ScreenType g_currentScreen = SCREEN_CLOCK;

// Heart rate simulation
int g_heartRate = 70;
double g_lastHeartUpdate = 0.0;

// Battery simulation
int g_batteryPercent = 100;
double g_lastBatteryUpdate = 0.0;

// Mouse callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (g_handController && g_handController->isInViewingMode()) {
        return;  // No camera movement in viewing mode
    }
    
    if (g_firstMouse) {
        g_lastMouseY = ypos;
        g_firstMouse = false;
        return;
    }
    
    double yoffset = g_lastMouseY - ypos;
    g_lastMouseY = ypos;
    
    if (g_camera) {
        g_camera->moveVertical(yoffset * 0.005f);
    }
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (!g_handController || !g_handController->isInViewingMode()) {
        return;  // Only handle clicks in viewing mode
    }
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // Simple click detection for screen navigation
        // Left third = previous screen, right third = next screen
        if (xpos < wWidth / 3.0) {
            // Previous screen
            if (g_currentScreen == SCREEN_HEART_RATE) g_currentScreen = SCREEN_CLOCK;
            else if (g_currentScreen == SCREEN_BATTERY) g_currentScreen = SCREEN_HEART_RATE;
        } else if (xpos > 2.0 * wWidth / 3.0) {
            // Next screen
            if (g_currentScreen == SCREEN_CLOCK) g_currentScreen = SCREEN_HEART_RATE;
            else if (g_currentScreen == SCREEN_HEART_RATE) g_currentScreen = SCREEN_BATTERY;
        }
    }
}

// Key callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (g_handController) {
            g_handController->toggleState();
            
            // Toggle cursor visibility
            if (g_handController->isInViewingMode()) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
    }
    
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// Render a simple colored quad (for screen content placeholder)
void renderScreenContent(unsigned int shader, ScreenType screen, double currentTime) {
    // This is a simplified version - in full implementation, this would render
    // the actual watch UI with text, graphics, etc.
    
    glm::vec3 bgColor;
    switch (screen) {
        case SCREEN_CLOCK:
            bgColor = glm::vec3(0.1f, 0.1f, 0.15f);
            break;
        case SCREEN_HEART_RATE:
            bgColor = glm::vec3(0.15f, 0.05f, 0.05f);
            break;
        case SCREEN_BATTERY:
            bgColor = glm::vec3(0.05f, 0.15f, 0.05f);
            break;
    }
    
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(bgColor));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(bgColor));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.1f)));
    glUniform1f(glGetUniformLocation(shader, "uMaterial.shine"), 32.0f);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return endProgram("GLFW failed to initialize");
    }
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(wWidth, wHeight, "SmartWatch3D", NULL, NULL);
    if (window == NULL) {
        return endProgram("Window creation failed");
    }
    glfwMakeContextCurrent(window);
    
    // Set callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        return endProgram("GLEW initialization failed");
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    
    // Create shaders
    unsigned int phongShader = createShader("phong.vert", "phong.frag");
    unsigned int screenShader = createShader("Shaders/screen.vert", "Shaders/screen.frag");
    
    // Load textures
    // unsigned int roadTexture = loadImageToTexture("Resources/road.jpg");  // Too large, causes access violation
    unsigned int roadTexture = 0;  // Use solid color for now
    
    // Initialize systems
    g_camera = new Camera(glm::vec3(0.0f, 2.0f, 5.0f), (float)wWidth / (float)wHeight);
    g_handController = new HandController();
    g_runningSimulation = new RunningSimulation(10.0f, 5);
    
    // Create geometry
    Mesh groundPlane = Geometry::createGroundPlane(50.0f, 50.0f, 20);
    Mesh handModel = Geometry::createHandModel();
    Mesh watchScreen = Geometry::createWatchScreen(0.3f);
    Mesh roadSegment = Geometry::createRoadSegment(5.0f, 10.0f);
    
    // Load building models
    std::vector<Model*> buildings;
    buildings.push_back(new Model("Resources/ChonkyBuilding/chonky_buildingA.obj"));
    buildings.push_back(new Model("Resources/Skyscraper/skyscraperE.obj"));
    buildings.push_back(new Model("Resources/TallBuilding/tall_buildingC.obj"));
    
    // Building positions (along the road)
    std::vector<glm::vec3> buildingPositions = {
        glm::vec3(-8.0f, 0.0f, -10.0f),
        glm::vec3(8.0f, 0.0f, -15.0f),
        glm::vec3(-8.0f, 0.0f, -25.0f),
        glm::vec3(8.0f, 0.0f, -35.0f),
        glm::vec3(-8.0f, 0.0f, -45.0f)
    };
    
    // Lighting setup
    glm::vec3 lightPos(0.0f, 10.0f, 0.0f);
    glm::vec3 lightAmbient(0.2f, 0.2f, 0.2f);
    glm::vec3 lightDiffuse(0.8f, 0.8f, 0.8f);
    glm::vec3 lightSpecular(1.0f, 1.0f, 1.0f);
    
    double lastTime = glfwGetTime();
    double deltaTime = 0.0;
    
    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Handle running input
        g_isRunning = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) && 
                      (g_currentScreen == SCREEN_HEART_RATE) &&
                      (g_handController->isInViewingMode());
        
        // Update systems
        g_handController->update(deltaTime);
        g_camera->updateBobbing(deltaTime, g_isRunning);
        g_runningSimulation->update(deltaTime, g_isRunning);
        
        // Update heart rate
        if (currentTime - g_lastHeartUpdate >= 0.1) {
            g_lastHeartUpdate = currentTime;
            if (g_isRunning) {
                g_heartRate = std::min(220, g_heartRate + 2);
            } else {
                if (g_heartRate > 70) {
                    g_heartRate = std::max(70, g_heartRate - 1);
                }
            }
        }
        
        // Update battery
        if (currentTime - g_lastBatteryUpdate >= 10.0) {
            g_lastBatteryUpdate = currentTime;
            g_batteryPercent = std::max(0, g_batteryPercent - 1);
        }
        
        // Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Get matrices
        glm::mat4 view = g_camera->getViewMatrix();
        glm::mat4 projection = g_camera->getProjectionMatrix();
        glm::vec3 cameraPos = g_camera->getPosition();
        
        // Use phong shader for most objects
        glUseProgram(phongShader);
        
        // Set view and projection matrices
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(phongShader, "uViewPos"), 1, glm::value_ptr(cameraPos));
        
        // Set lighting
        glUniform3fv(glGetUniformLocation(phongShader, "uLight.pos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(phongShader, "uLight.kA"), 1, glm::value_ptr(lightAmbient));
        glUniform3fv(glGetUniformLocation(phongShader, "uLight.kD"), 1, glm::value_ptr(lightDiffuse));
        glUniform3fv(glGetUniformLocation(phongShader, "uLight.kS"), 1, glm::value_ptr(lightSpecular));
        
        // Render ground plane with road texture
        glm::mat4 groundModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(groundModel));
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, roadTexture);
        glUniform1i(glGetUniformLocation(phongShader, "uTexture"), 0);
        
        glm::vec3 groundMat(0.5f, 0.5f, 0.5f);
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(groundMat));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(groundMat));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.2f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 32.0f);
        
        groundPlane.draw();
        
        // Render road segments (for running simulation)
        const auto& segmentPositions = g_runningSimulation->getSegmentPositions();
        for (float zPos : segmentPositions) {
            glm::mat4 segmentModel = glm::mat4(1.0f);
            segmentModel = glm::translate(segmentModel, glm::vec3(0.0f, 0.01f, zPos));
            glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(segmentModel));
            roadSegment.draw();
        }
        
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 0);
        
        // Render buildings
        for (size_t i = 0; i < buildings.size() && i < buildingPositions.size(); i++) {
            glm::mat4 buildingModel = glm::mat4(1.0f);
            buildingModel = glm::translate(buildingModel, buildingPositions[i]);
            buildingModel = glm::scale(buildingModel, glm::vec3(0.5f));
            glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(buildingModel));
            
            glm::vec3 buildingMat(0.6f, 0.6f, 0.7f);
            glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(buildingMat));
            glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(buildingMat));
            glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.3f)));
            glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 64.0f);
            
            buildings[i]->draw();
        }
        
        // Render hand
        glm::mat4 handM = g_handController->getTransformMatrix();
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(handM));
        
        glm::vec3 handMat(0.8f, 0.7f, 0.6f);  // Skin tone
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(handMat));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(handMat));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.2f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 32.0f);
        
        handModel.draw();
        
        // Render watch screen (emissive)
        glm::mat4 watchModel = g_handController->getTransformMatrix();
        watchModel = glm::translate(watchModel, glm::vec3(0.0f, 0.0f, -0.4f));
        
        // Render screen background with phong shader first
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(watchModel));
        renderScreenContent(phongShader, g_currentScreen, currentTime);
        watchScreen.draw();
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    groundPlane.cleanup();
    handModel.cleanup();
    watchScreen.cleanup();
    roadSegment.cleanup();
    
    for (auto* building : buildings) {
        delete building;
    }
    
    delete g_camera;
    delete g_handController;
    delete g_runningSimulation;
    
    glDeleteProgram(phongShader);
    glDeleteProgram(screenShader);
    if (roadTexture != 0) {
        glDeleteTextures(1, &roadTexture);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}