#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <thread>
#include <chrono>

#include "../Header/Util.h"
#include "../Header/Camera.h"
#include "../Header/ShaderUniforms.h"
#include "../Header/Sun.h"
#include "../Header/Street.h"
#include "../Header/Hand.h"
#include "../Header/Watch.h"
#include "../Header/DigitRenderer.h"

// FPS limiting
const int TARGET_FPS = 75;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

// Window dimensions
int g_width = 1200, g_height = 800;

// Core objects
Camera* g_camera = nullptr;
Hand* g_hand = nullptr;
Watch* g_watch = nullptr;
Sun* g_sun = nullptr;
Street* g_street = nullptr;
DigitRenderer* g_digitRenderer = nullptr;
ShaderUniforms g_uniforms;

// Input state
double g_lastMouseX = 0.0, g_lastMouseY = 0.0;
bool g_firstMouse = true;
bool g_isRunning = false;
bool g_freeCameraMode = false;

// Cursor
GLFWcursor* g_heartCursor = nullptr;

// Arrow positions for click detection
glm::vec2 g_leftArrowScreenPos(0.0f);
glm::vec2 g_rightArrowScreenPos(0.0f);
const float ARROW_CLICK_RADIUS = 50.0f;

// Project 3D point to screen space
glm::vec2 projectToScreen(const glm::vec3& worldPos, const glm::mat4& view, const glm::mat4& proj) {
    glm::vec4 clipPos = proj * view * glm::vec4(worldPos, 1.0f);
    if (clipPos.w <= 0.0f) return glm::vec2(-1000.0f);
    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
    return glm::vec2((ndc.x + 1.0f) * 0.5f * g_width, (1.0f - ndc.y) * 0.5f * g_height);
}

// Mouse movement callback
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (g_hand && g_hand->isInViewingMode() && !g_freeCameraMode) return;

    if (g_firstMouse) {
        g_lastMouseX = xpos;
        g_lastMouseY = ypos;
        g_firstMouse = false;
        return;
    }

    double xoffset = xpos - g_lastMouseX;
    double yoffset = g_lastMouseY - ypos;
    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    if (g_camera) {
        if (g_freeCameraMode) {
            g_camera->rotate((float)xoffset, (float)yoffset);
        } else {
            g_camera->moveVertical((float)yoffset * 0.005f);
        }
    }
}

// Mouse button callback
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!g_hand || !g_hand->isInViewingMode() || !g_watch) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        glm::vec2 mousePos((float)xpos, (float)ypos);

        if (glm::length(mousePos - g_leftArrowScreenPos) < ARROW_CLICK_RADIUS) {
            g_watch->prevScreen();
        } else if (glm::length(mousePos - g_rightArrowScreenPos) < ARROW_CLICK_RADIUS) {
            g_watch->nextScreen();
        }
    }
}

// Key callback
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && g_hand) {
        g_hand->toggleViewingMode();
        g_firstMouse = true;
    }
    if (key == GLFW_KEY_K && action == GLFW_PRESS) {
        g_freeCameraMode = !g_freeCameraMode;
        g_firstMouse = true;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

// Render student info overlay
void renderStudentInfoOverlay(unsigned int shader) {
    glm::mat4 orthoProj = glm::ortho(0.0f, (float)g_width, 0.0f, (float)g_height, -1.0f, 1.0f);
    glm::mat4 identity = glm::mat4(1.0f);

    g_uniforms.setViewMatrix(identity);
    g_uniforms.setProjectionMatrix(orthoProj);

    glDisable(GL_DEPTH_TEST);
    g_uniforms.setFog(false);

    float margin = 20.0f, textScale = 25.0f;
    float bgWidth = 280.0f, bgHeight = 90.0f;

    // Background
    glm::mat4 bgModel = glm::translate(glm::mat4(1.0f), glm::vec3(margin + bgWidth/2.0f, margin + bgHeight/2.0f, -0.1f));
    bgModel = glm::scale(bgModel, glm::vec3(bgWidth, bgHeight, 1.0f));
    g_uniforms.setModelMatrix(bgModel);
    g_uniforms.setMaterial(glm::vec3(0.0f), glm::vec3(0.15f, 0.15f, 0.2f), glm::vec3(0.0f), 1.0f);
    g_uniforms.setTexture(false);

    static unsigned int bgVAO = 0;
    if (bgVAO == 0) {
        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.0f
        };
        unsigned int bgVBO;
        glGenVertexArrays(1, &bgVAO);
        glGenBuffers(1, &bgVBO);
        glBindVertexArray(bgVAO);
        glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }
    glBindVertexArray(bgVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Text
    glm::vec3 textColor(0.0f, 1.0f, 0.5f);
    g_digitRenderer->drawText("papp tamas", margin + 10.0f, margin + 55.0f, textScale, textColor, shader, identity);
    g_digitRenderer->drawText("ra-4-2022", margin + 10.0f, margin + 20.0f, textScale, textColor, shader, identity);

    glEnable(GL_DEPTH_TEST);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Fullscreen
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    g_width = mode->width;
    g_height = mode->height;

    GLFWwindow* window = glfwCreateWindow(g_width, g_height, "SmartWatch3D", monitor, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Disable vsync for manual FPS control

    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.07f, 0.08f, 0.12f, 1.0f);

    // Create shader and cache uniform locations
    unsigned int shader = createShader("phong.vert", "phong.frag");
    g_uniforms.init(shader);

    // Load cursor
    g_heartCursor = loadImageToCursor("Resources/textures/red_heart_cursor.png");
    if (g_heartCursor) glfwSetCursor(window, g_heartCursor);

    // Initialize objects
    g_camera = new Camera(glm::vec3(0.0f, 1.4f, 0.0f), (float)g_width / (float)g_height);

    g_sun = new Sun();
    g_sun->init("Resources/sun/2k_sun.jpg");

    g_street = new Street();
    g_street->init(8.0f, 15.0f, 12);

    g_hand = new Hand();
    g_hand->init("Resources/arm/arm.obj");

    g_watch = new Watch();
    g_watch->init();

    g_digitRenderer = new DigitRenderer();
    g_digitRenderer->init();

    // Watch light parameters
    glm::vec3 watchLightAmbient(0.005f, 0.005f, 0.008f);
    glm::vec3 watchLightDiffuse(0.03f, 0.04f, 0.05f);
    glm::vec3 watchLightSpecular(0.02f, 0.02f, 0.03f);

    double lastTime = glfwGetTime();

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Free camera movement
        if (g_freeCameraMode) {
            float speed = 5.0f * (float)deltaTime;
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) g_camera->moveForward(speed);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) g_camera->moveForward(-speed);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) g_camera->moveRight(-speed);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) g_camera->moveRight(speed);
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) g_camera->moveUp(-speed);
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) g_camera->moveUp(speed);
        }

        // Running state
        g_isRunning = !g_freeCameraMode &&
                      glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS &&
                      g_watch->getCurrentScreen() == WATCH_SCREEN_HEART_RATE &&
                      g_hand->isInViewingMode();

        // Update objects
        g_camera->updateBobbing(deltaTime, g_isRunning);
        glm::vec3 camPos = g_camera->getPosition();

        g_hand->update(deltaTime, camPos);
        g_street->update(deltaTime, g_isRunning);
        g_watch->update(deltaTime, currentTime, g_isRunning);

        // Cursor visibility: hidden normally, heart when viewing watch,
        // disabled (captured) in free-camera so rotation has no edge limit.
        if (g_freeCameraMode) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (g_hand && g_hand->isInViewingMode()) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            if (g_heartCursor) glfwSetCursor(window, g_heartCursor);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }

        // Render
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 view = g_camera->getViewMatrix();
        glm::mat4 projection = g_camera->getProjectionMatrix();

        g_uniforms.setViewMatrix(view);
        g_uniforms.setProjectionMatrix(projection);
        glUniform3fv(g_uniforms.uViewPos, 1, glm::value_ptr(camPos));

        // Set main light (sun)
        glm::vec3 lightPos = g_sun->getPosition();
        glUniform3fv(g_uniforms.uLight_pos, 1, glm::value_ptr(lightPos));
        glUniform3fv(g_uniforms.uLight_kA, 1, glm::value_ptr(g_sun->getAmbient()));
        glUniform3fv(g_uniforms.uLight_kD, 1, glm::value_ptr(g_sun->getDiffuse()));
        glUniform3fv(g_uniforms.uLight_kS, 1, glm::value_ptr(g_sun->getSpecular()));

        // Set watch light
        glm::vec3 watchScreenPos = g_watch->getScreenPosition(g_hand->getTransformMatrix());
        glUniform3fv(g_uniforms.uWatchLight_pos, 1, glm::value_ptr(watchScreenPos));
        glUniform3fv(g_uniforms.uWatchLight_kA, 1, glm::value_ptr(watchLightAmbient));
        glUniform3fv(g_uniforms.uWatchLight_kD, 1, glm::value_ptr(watchLightDiffuse));
        glUniform3fv(g_uniforms.uWatchLight_kS, 1, glm::value_ptr(watchLightSpecular));
        glUniform1i(g_uniforms.uUseWatchLight, g_hand->isInViewingMode() ? 1 : 0);

        // Set fog
        g_uniforms.setFog(true, glm::vec3(0.07f, 0.08f, 0.12f), 0.00025f);

        // Render street (ground, road, buildings)
        g_street->render(g_uniforms);

        // Render sun (no fog, emissive)
        g_sun->render(g_uniforms);
        g_uniforms.setFog(true, glm::vec3(0.07f, 0.08f, 0.12f), 0.00025f);

        // Render hand and watch (no fog - close to camera)
        g_uniforms.setFog(false);
        g_hand->render(g_uniforms);
        g_watch->render(g_uniforms, g_hand->getTransformMatrix(), shader);

        // Calculate arrow positions for click detection
        glm::mat4 handM = g_hand->getTransformMatrix();
        glm::mat4 watchM = glm::translate(handM, glm::vec3(-0.15f, 0.0f, -0.05f));
        watchM = glm::rotate(watchM, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        watchM = glm::scale(watchM, glm::vec3(0.35f));
        glm::mat4 screenM = glm::translate(watchM, glm::vec3(0.0f, 0.0f, 0.021f));
        float contentScale = 0.55f;

        glm::vec3 leftArrowWorld = glm::vec3(screenM * glm::vec4(-0.14f * contentScale, 0.0f, 0.01f, 1.0f));
        glm::vec3 rightArrowWorld = glm::vec3(screenM * glm::vec4(0.14f * contentScale, 0.0f, 0.01f, 1.0f));
        g_leftArrowScreenPos = projectToScreen(leftArrowWorld, view, projection);
        g_rightArrowScreenPos = projectToScreen(rightArrowWorld, view, projection);

        // Render student info overlay
        renderStudentInfoOverlay(shader);

        // Restore matrices for next frame
        g_uniforms.setViewMatrix(view);
        g_uniforms.setProjectionMatrix(projection);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // FPS limiting
        double frameEndTime = glfwGetTime();
        double frameTime = frameEndTime - currentTime;
        if (frameTime < TARGET_FRAME_TIME) {
            std::this_thread::sleep_for(std::chrono::microseconds((int)((TARGET_FRAME_TIME - frameTime) * 1000000)));
        }
    }

    // Cleanup
    delete g_camera;
    delete g_sun;
    delete g_street;
    delete g_hand;
    delete g_watch;
    delete g_digitRenderer;

    glDeleteProgram(shader);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
