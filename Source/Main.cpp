#define STB_IMAGE_IMPLEMENTATION
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

// Simulation State
int g_heartRate = 70;
double g_lastHeartUpdate = 0.0;
int g_batteryPercent = 100;
double g_lastBatteryUpdate = 0.0;

// Time Simulation
int g_hours = 12, g_minutes = 30, g_seconds = 0;
double g_lastTimeUpdate = 0.0;

// Resources
unsigned int g_roadTexture = 0;
unsigned int g_warningTexture = 0;
unsigned int g_ecgTexture = 0;
unsigned int g_batteryTexture = 0;
unsigned int g_arrowRightTexture = 0;

GLFWcursor* g_heartCursor = nullptr;

// Mouse callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (g_handController && g_handController->isInViewingMode()) {
        return; 
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
        return;
    }
    
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        // Simple click regions
        if (xpos < wWidth / 3.0) {
            // Left Arrow Area
            if (g_currentScreen == SCREEN_HEART_RATE) g_currentScreen = SCREEN_CLOCK;
            else if (g_currentScreen == SCREEN_BATTERY) g_currentScreen = SCREEN_HEART_RATE;
        } else if (xpos > 2.0 * wWidth / 3.0) {
            // Right Arrow Area
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
            
            if (g_handController->isInViewingMode()) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                if (g_heartCursor) glfwSetCursor(window, g_heartCursor);
            } else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
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
    
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(1.0f)));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(1.0f)));
    
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

void renderScreenContent(unsigned int shader, ScreenType screen, double currentTime, const glm::mat4& parentModel) {
    // Background
    glm::vec3 bgColor(0.05f, 0.05f, 0.05f);
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(bgColor));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(bgColor));
    
    // Draw arrows
    if (screen != SCREEN_CLOCK) {
        renderQuad(shader, g_arrowRightTexture, -0.12f, 0.0f, 0.05f, 0.05f, parentModel, true);
    }
    if (screen != SCREEN_BATTERY) {
        renderQuad(shader, g_arrowRightTexture, 0.12f, 0.0f, 0.05f, 0.05f, parentModel, false);
    }

    if (screen == SCREEN_CLOCK) {
        float scale = 0.04f;
        float totalWidth = (6 * 0.6f + 2 * 0.3f) * scale;
        g_digitRenderer->drawTime(g_hours, g_minutes, g_seconds, -totalWidth/2.0f, -0.02f, scale, glm::vec3(1.0f), shader, parentModel);
    }
    else if (screen == SCREEN_HEART_RATE) {
        // ECG
        float speed = g_isRunning ? 2.0f : 0.5f;
        float offset = (float)currentTime * speed;
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_ecgTexture);
        
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glTranslatef(offset, 0.0f, 0.0f);
        if (g_isRunning) glScalef(2.0f, 1.0f, 1.0f); 
        glMatrixMode(GL_MODELVIEW);
        
        renderQuad(shader, g_ecgTexture, 0.0f, -0.05f, 0.2f, 0.1f, parentModel);
        
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);

        float scale = 0.03f;
        g_digitRenderer->drawNumber(g_heartRate, -0.05f, 0.05f, scale, glm::vec3(1.0f, 0.0f, 0.0f), shader, parentModel);
        
        if (g_heartRate > 200) {
            renderQuad(shader, g_warningTexture, 0.0f, 0.0f, 0.25f, 0.25f, parentModel);
        }
    }
    else if (screen == SCREEN_BATTERY) {
        renderQuad(shader, g_batteryTexture, 0.0f, 0.0f, 0.15f, 0.08f, parentModel);
        
        float barWidth = 0.12f * (g_batteryPercent / 100.0f);
        
        glm::vec3 barColor(0.0f, 1.0f, 0.0f);
        if (g_batteryPercent < 20) barColor = glm::vec3(1.0f, 1.0f, 0.0f);
        if (g_batteryPercent < 10) barColor = glm::vec3(1.0f, 0.0f, 0.0f);
        
        glm::mat4 model = parentModel;
        model = glm::translate(model, glm::vec3(-0.06f + barWidth/2.0f, 0.0f, 0.02f));
        model = glm::scale(model, glm::vec3(barWidth, 0.06f, 1.0f));
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

        float scale = 0.03f;
        g_digitRenderer->drawNumber(g_batteryPercent, -0.02f, 0.06f, scale, glm::vec3(1.0f), shader, parentModel);
        g_digitRenderer->drawPercent(0.04f, 0.06f, scale, glm::vec3(1.0f), shader, parentModel);
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    if (glewInit() != GLEW_OK) return endProgram("GLEW initialization failed");
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    
    unsigned int phongShader = createShader("phong.vert", "phong.frag");

    g_roadTexture = loadImageToTexture("Resources/road.jpg");
    g_warningTexture = loadImageToTexture("Resources/textures/warning.png");
    g_ecgTexture = loadImageToTexture("Resources/textures/ecg_wave.png");
    g_batteryTexture = loadImageToTexture("Resources/textures/battery.png");
    g_arrowRightTexture = loadImageToTexture("Resources/textures/arrow_right.png");
    g_heartCursor = loadImageToCursor("Resources/textures/red_heart_cursor.png");
    
    g_camera = new Camera(glm::vec3(0.0f, 2.0f, 5.0f), (float)wWidth / (float)wHeight);
    g_handController = new HandController();
    g_runningSimulation = new RunningSimulation(10.0f, 5);
    g_digitRenderer = new DigitRenderer();
    g_digitRenderer->init();
    
    Mesh groundPlane = Geometry::createGroundPlane(50.0f, 50.0f, 20);
    Mesh handModel = Geometry::createHandModel();
    Mesh watchScreen = Geometry::createWatchScreen(0.3f);
    Mesh roadSegment = Geometry::createRoadSegment(5.0f, 10.0f);
    
    std::vector<Model*> buildings;
    buildings.push_back(new Model("Resources/ChonkyBuilding/chonky_buildingA.obj"));
    buildings.push_back(new Model("Resources/Skyscraper/skyscraperE.obj"));
    buildings.push_back(new Model("Resources/TallBuilding/tall_buildingC.obj"));
    
    std::vector<glm::vec3> buildingPositions = {
        glm::vec3(-8.0f, 0.0f, -10.0f),
        glm::vec3(8.0f, 0.0f, -15.0f),
        glm::vec3(-8.0f, 0.0f, -25.0f),
        glm::vec3(8.0f, 0.0f, -35.0f),
        glm::vec3(-8.0f, 0.0f, -45.0f)
    };
    
    glm::vec3 lightPos(0.0f, 10.0f, 0.0f);
    glm::vec3 lightAmbient(0.2f, 0.2f, 0.2f);
    glm::vec3 lightDiffuse(0.8f, 0.8f, 0.8f);
    glm::vec3 lightSpecular(1.0f, 1.0f, 1.0f);
    
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        g_isRunning = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) &&
                      (g_currentScreen == SCREEN_HEART_RATE) &&
                      (g_handController->isInViewingMode());
        
        g_handController->update(deltaTime);
        g_camera->updateBobbing(deltaTime, g_isRunning);
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
        
        // Render Ground
        glm::mat4 groundModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(groundModel));
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_roadTexture);
        glUniform1i(glGetUniformLocation(phongShader, "uTexture"), 0);
        groundPlane.draw();
        
        // Render Road Segments
        for (float zPos : g_runningSimulation->getSegmentPositions()) {
            glm::mat4 segmentModel = glm::mat4(1.0f);
            segmentModel = glm::translate(segmentModel, glm::vec3(0.0f, 0.01f, zPos));
            glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(segmentModel));
            roadSegment.draw();
        }
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 0);
        
        // Render Buildings
        int bIdx = 0;
        for (auto* building : buildings) {
            if (bIdx >= buildingPositions.size()) break;
            glm::mat4 bModel = glm::mat4(1.0f);
            bModel = glm::translate(bModel, buildingPositions[bIdx++]);
            bModel = glm::scale(bModel, glm::vec3(0.5f));
            glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(bModel));
            building->draw();
        }
        
        // Render Hand & Watch
        glm::mat4 handM = g_handController->getTransformMatrix();
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(handM));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.8f, 0.7f, 0.6f)));
        handModel.draw();
        
        // Watch Screen Body
        glm::mat4 watchM = handM;
        watchM = glm::translate(watchM, glm::vec3(0.0f, 0.0f, -0.4f)); 
        
        // Render screen content relative to watchM
        // Note: We need to draw the screen background first?
        // renderScreenContent draws background.
        // It draws relative to watchM.
        
        // But watchM is where the screen IS.
        // renderScreenContent should use watchM as parent.
        renderScreenContent(phongShader, g_currentScreen, currentTime, watchM);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    groundPlane.cleanup();
    handModel.cleanup();
    watchScreen.cleanup();
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