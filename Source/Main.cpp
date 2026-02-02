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

// Mouse callback - only move camera when NOT viewing watch
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    // When viewing watch, don't process mouse movement for camera
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
                // Show cursor with heart icon when viewing watch
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                if (g_heartCursor) glfwSetCursor(window, g_heartCursor);
            } else {
                // Hide cursor for camera movement mode
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                glfwSetCursor(window, NULL);
                // Reset mouse tracking to prevent camera jump
                g_firstMouse = true;
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

void renderScreenContent(unsigned int shader, ScreenType screen, double currentTime, const glm::mat4& parentModel, float contentScale = 1.0f) {
    // Background
    glm::vec3 bgColor(0.05f, 0.05f, 0.05f);
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(bgColor));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(bgColor));

    // Scale all positions and sizes by contentScale
    float s = contentScale;

    // Draw arrows
    if (screen != SCREEN_CLOCK) {
        renderQuad(shader, g_arrowRightTexture, -0.18f * s, 0.0f, 0.07f * s, 0.07f * s, parentModel, true);
    }
    if (screen != SCREEN_BATTERY) {
        renderQuad(shader, g_arrowRightTexture, 0.18f * s, 0.0f, 0.07f * s, 0.07f * s, parentModel, false);
    }

    if (screen == SCREEN_CLOCK) {
        float scale = 0.055f * s;
        float totalWidth = (6 * 0.6f + 2 * 0.3f) * scale;
        g_digitRenderer->drawTime(g_hours, g_minutes, g_seconds, -totalWidth/2.0f, -0.03f * s, scale, glm::vec3(1.0f), shader, parentModel);
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

        renderQuad(shader, g_ecgTexture, 0.0f, -0.07f * s, 0.28f * s, 0.14f * s, parentModel);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);

        float scale = 0.04f * s;
        g_digitRenderer->drawNumber(g_heartRate, -0.07f * s, 0.07f * s, scale, glm::vec3(1.0f, 0.0f, 0.0f), shader, parentModel);

        if (g_heartRate > 200) {
            renderQuad(shader, g_warningTexture, 0.0f, 0.0f, 0.35f * s, 0.35f * s, parentModel);
        }
    }
    else if (screen == SCREEN_BATTERY) {
        renderQuad(shader, g_batteryTexture, 0.0f, 0.0f, 0.2f * s, 0.11f * s, parentModel);

        float barWidth = 0.16f * s * (g_batteryPercent / 100.0f);

        glm::vec3 barColor(0.0f, 1.0f, 0.0f);
        if (g_batteryPercent < 20) barColor = glm::vec3(1.0f, 1.0f, 0.0f);
        if (g_batteryPercent < 10) barColor = glm::vec3(1.0f, 0.0f, 0.0f);

        glm::mat4 model = parentModel;
        model = glm::translate(model, glm::vec3(-0.08f * s + barWidth/2.0f, 0.0f, 0.02f));
        model = glm::scale(model, glm::vec3(barWidth, 0.08f * s, 1.0f));
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

        float scale = 0.04f * s;
        g_digitRenderer->drawNumber(g_batteryPercent, -0.03f * s, 0.09f * s, scale, glm::vec3(1.0f), shader, parentModel);
        g_digitRenderer->drawPercent(0.05f * s, 0.09f * s, scale, glm::vec3(1.0f), shader, parentModel);
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
    // Start with hidden cursor - still tracks mouse for camera movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    
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
    g_heartCursor = loadImageToCursor("Resources/textures/red_heart_cursor.png");
    
    // Camera positioned low in the street, close to hand level
    g_camera = new Camera(glm::vec3(0.0f, 1.4f, 0.0f), (float)wWidth / (float)wHeight);
    g_handController = new HandController();
    // Larger segments to match wider road, more segments for extended scene
    g_runningSimulation = new RunningSimulation(15.0f, 8);
    g_digitRenderer = new DigitRenderer();
    g_digitRenderer->init();
    
    // Extended ground plane to accommodate more buildings
    Mesh groundPlane = Geometry::createGroundPlane(80.0f, 250.0f, 40);
    Mesh handModel = Geometry::createHandModel();
    // Watch screen for display - larger to fill view
    Mesh watchScreen = Geometry::createWatchScreen(0.42f);
    // Black watch body/case around the screen - larger
    Mesh watchBody = Geometry::createWatchBody(0.5f, 0.5f, 0.06f);
    // Wider road for better perspective
    Mesh roadSegment = Geometry::createRoadSegment(8.0f, 15.0f);
    
    std::vector<Model*> buildings;
    buildings.push_back(new Model("Resources/ChonkyBuilding/chonky_buildingA.obj"));
    buildings.push_back(new Model("Resources/Skyscraper/skyscraperE.obj"));
    buildings.push_back(new Model("Resources/TallBuilding/tall_buildingC.obj"));
    buildings.push_back(new Model("Resources/Large Building/large_buildingE.obj"));

    // Dense building placement along both sides of the road
    // Buildings are larger (scale 3.0-5.0) so they tower over the camera
    std::vector<glm::vec3> buildingPositions;
    std::vector<float> buildingScales;
    std::vector<int> buildingTypes; // Which model to use

    // Create dense rows of buildings on both sides of the road
    float roadHalfWidth = 4.0f; // Road is 8 units wide, buildings start at edge
    float buildingSpacing = 10.0f; // Space between buildings along Z
    int numBuildingsPerSide = 18; // More buildings for density

    for (int i = 0; i < numBuildingsPerSide; i++) {
        float zPos = -3.0f - i * buildingSpacing; // Start close to camera at z=0

        // Left side buildings
        buildingPositions.push_back(glm::vec3(-roadHalfWidth - 5.0f, 0.0f, zPos));
        buildingScales.push_back(3.5f + (i % 3) * 1.0f); // Vary scale 3.5 - 5.5
        buildingTypes.push_back(i % 4); // Cycle through building types

        // Right side buildings
        buildingPositions.push_back(glm::vec3(roadHalfWidth + 5.0f, 0.0f, zPos - buildingSpacing * 0.5f));
        buildingScales.push_back(3.0f + ((i + 1) % 4) * 0.9f); // Vary scale 3.0 - 5.7
        buildingTypes.push_back((i + 2) % 4);
    }

    // Main sky/sun light - large and elevated for dramatic lighting
    glm::vec3 lightPos(50.0f, 100.0f, 30.0f); // High in the sky, slightly behind camera
    glm::vec3 lightAmbient(0.25f, 0.25f, 0.28f); // Slightly blue ambient for outdoor feel
    glm::vec3 lightDiffuse(1.0f, 0.95f, 0.85f); // Warm sunlight
    glm::vec3 lightSpecular(1.0f, 1.0f, 0.9f);

    // Watch screen light parameters (weak secondary light)
    glm::vec3 watchLightAmbient(0.02f, 0.02f, 0.03f);
    glm::vec3 watchLightDiffuse(0.15f, 0.18f, 0.2f); // Cool blue-ish screen glow
    glm::vec3 watchLightSpecular(0.1f, 0.1f, 0.15f);
    
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        g_isRunning = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) &&
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
        glm::vec3 watchScreenPos = glm::vec3(handM_temp * glm::vec4(0.0f, 0.08f, -1.17f, 1.0f));
        glUniform3fv(glGetUniformLocation(phongShader, "uWatchLight.pos"), 1, glm::value_ptr(watchScreenPos));
        glUniform3fv(glGetUniformLocation(phongShader, "uWatchLight.kA"), 1, glm::value_ptr(watchLightAmbient));
        glUniform3fv(glGetUniformLocation(phongShader, "uWatchLight.kD"), 1, glm::value_ptr(watchLightDiffuse));
        glUniform3fv(glGetUniformLocation(phongShader, "uWatchLight.kS"), 1, glm::value_ptr(watchLightSpecular));
        glUniform1i(glGetUniformLocation(phongShader, "uUseWatchLight"), g_handController->isInViewingMode() ? 1 : 0);
        
        // Render Ground - with proper material for outdoor ground
        glm::mat4 groundModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(groundModel));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.7f, 0.65f, 0.55f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.3f, 0.28f, 0.25f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.1f, 0.1f, 0.1f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 4.0f);
        glUniform1i(glGetUniformLocation(phongShader, "uUseTexture"), 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_roadTexture);
        glUniform1i(glGetUniformLocation(phongShader, "uTexture"), 0);
        groundPlane.draw();
        
        // Render Road Segments - darker asphalt-like material
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.4f, 0.4f, 0.42f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.15f, 0.15f, 0.17f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.15f, 0.15f, 0.15f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 8.0f);
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

        for (size_t i = 0; i < buildingPositions.size(); i++) {
            glm::mat4 bModel = glm::mat4(1.0f);
            bModel = glm::translate(bModel, buildingPositions[i]);
            bModel = glm::scale(bModel, glm::vec3(buildingScales[i]));
            glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(bModel));
            buildings[buildingTypes[i]]->draw();
        }
        
        // Render Hand - skin-like material
        glm::mat4 handM = g_handController->getTransformMatrix();
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(handM));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.85f, 0.72f, 0.62f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.4f, 0.32f, 0.28f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.25f, 0.22f, 0.2f)));
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 12.0f);
        handModel.draw();

        // Watch position on wrist - middle of the long arm, rotated around Z axis
        glm::mat4 watchM = handM;
        watchM = glm::translate(watchM, glm::vec3(0.0f, 0.08f, -1.2f)); // Middle of long arm
        watchM = glm::rotate(watchM, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around Z axis

        // Render black watch body/case
        glUniformMatrix4fv(glGetUniformLocation(phongShader, "uM"), 1, GL_FALSE, glm::value_ptr(watchM));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.05f, 0.05f, 0.05f))); // Black
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.02f, 0.02f, 0.02f)));
        glUniform3fv(glGetUniformLocation(phongShader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.4f, 0.4f, 0.4f))); // Shiny
        glUniform1f(glGetUniformLocation(phongShader, "uMaterial.shine"), 32.0f);
        watchBody.draw();

        // Watch screen (slightly in front of body toward camera)
        glm::mat4 screenM = watchM;
        screenM = glm::translate(screenM, glm::vec3(0.0f, 0.0f, 0.031f)); // Front of watch body

        // Scale content for watch screen
        float contentScale = 1.6f;

        // Render screen content
        renderScreenContent(phongShader, g_currentScreen, currentTime, screenM, contentScale);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    groundPlane.cleanup();
    handModel.cleanup();
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