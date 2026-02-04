#include "../Header/Watch.h"
#include "../Header/Util.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstdlib>

#include <ctime>

Watch::Watch()
    : digitRenderer(nullptr),
      currentScreen(WATCH_SCREEN_CLOCK),
      hours(12), minutes(30), seconds(0),
      lastTimeUpdate(0.0),
      heartRate(70),
      lastHeartUpdate(0.0),
      ecgScrollOffset(0.0f),
      batteryPercent(100),
      lastBatteryUpdate(0.0),
      warningTexture(0), ecgTexture(0), batteryTexture(0), arrowTexture(0),
      watchOffset(-0.15f, 0.0f, -0.05f),
      contentScale(0.55f) {
}

Watch::~Watch() {
    watchBody.cleanup();
    watchScreen.cleanup();
    delete digitRenderer;
}

void Watch::init() {
    watchBody = Geometry::createWatchBody(0.3f, 0.04f, 32);
    watchScreen = Geometry::createWatchScreen(0.25f, 32);

    digitRenderer = new DigitRenderer();
    digitRenderer->init();

    // Load textures
    warningTexture = loadImageToTexture("Resources/textures/warning.png");
    ecgTexture = loadImageToTexture("Resources/textures/ecg_wave.png");
    batteryTexture = loadImageToTexture("Resources/textures/battery.png");
    arrowTexture = loadImageToTexture("Resources/textures/arrow_right.png");

    // Seed clock from current system time
    time_t now = time(nullptr);
    struct tm lt;
    if (localtime_s(&lt, &now) == 0) {
        hours = lt.tm_hour;
        minutes = lt.tm_min;
        seconds = lt.tm_sec;
    }
}

void Watch::update(double deltaTime, double currentTime, bool isRunning) {
    // Update time
    if (currentTime - lastTimeUpdate >= 1.0) {
        lastTimeUpdate = currentTime;
        seconds++;
        if (seconds >= 60) { seconds = 0; minutes++; }
        if (minutes >= 60) { minutes = 0; hours++; }
        if (hours >= 24) hours = 0;
    }

    // Update heart rate
    if (currentTime - lastHeartUpdate >= (isRunning ? 0.05 : 0.1)) {
        lastHeartUpdate = currentTime;
        if (isRunning) {
            if (heartRate < 220) heartRate++;
        } else {
            if (heartRate > 70) heartRate--;
            else if (heartRate < 60) heartRate++;
            else heartRate += (rand() % 3 - 1);
        }
    }

    // Update ECG scroll
    float ecgSpeed = 0.3f * (heartRate / 70.0f);
    ecgScrollOffset += ecgSpeed * (float)deltaTime;
    if (ecgScrollOffset > 100.0f) ecgScrollOffset -= 100.0f;

    // Update battery
    if (currentTime - lastBatteryUpdate >= 10.0) {
        lastBatteryUpdate = currentTime;
        if (batteryPercent > 0) batteryPercent--;
    }
}

void Watch::render(const ShaderUniforms& uniforms, const glm::mat4& handMatrix, unsigned int shader) const {
    // Watch position relative to hand
    glm::mat4 watchM = handMatrix;
    watchM = glm::translate(watchM, watchOffset);
    watchM = glm::rotate(watchM, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Render watch body
    uniforms.setModelMatrix(watchM);
    uniforms.setMaterial(
        glm::vec3(0.02f),  // kD - very dark
        glm::vec3(0.01f),  // kA
        glm::vec3(0.1f),   // kS
        16.0f              // shine
    );
    uniforms.setTexture(false);
    watchBody.draw();

    // Render watch screen (slightly in front)
    glm::mat4 screenM = watchM;
    screenM = glm::translate(screenM, glm::vec3(0.0f, 0.0f, 0.021f));

    renderContent(shader, screenM, 0.0);
}

void Watch::renderContent(unsigned int shader, const glm::mat4& screenMatrix, double currentTime) const {
    float s = contentScale;

    // Draw white circular background
    glm::mat4 bgModel = screenMatrix;
    bgModel = glm::translate(bgModel, glm::vec3(0.0f, 0.0f, -0.001f));
    bgModel = glm::scale(bgModel, glm::vec3(0.23f, 0.23f, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader, "uM"), 1, GL_FALSE, glm::value_ptr(bgModel));
    glUniform1i(glGetUniformLocation(shader, "uUseTexture"), 0);
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.9f)));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.8f)));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kS"), 1, glm::value_ptr(glm::vec3(0.1f)));
    glUniform1f(glGetUniformLocation(shader, "uMaterial.shine"), 4.0f);

    // Draw circular background
    static unsigned int bgVAO = 0;
    static int bgVertexCount = 0;
    if (bgVAO == 0) {
        const int segments = 32;
        std::vector<float> fanVertices;
        for (int i = 0; i < segments; i++) {
            float angle1 = (float)i / segments * 2.0f * 3.14159265f;
            float angle2 = (float)(i + 1) / segments * 2.0f * 3.14159265f;
            // Center
            fanVertices.push_back(0.0f); fanVertices.push_back(0.0f); fanVertices.push_back(0.0f);
            // Edge 1
            fanVertices.push_back(0.5f * cos(angle1)); fanVertices.push_back(0.5f * sin(angle1)); fanVertices.push_back(0.0f);
            // Edge 2
            fanVertices.push_back(0.5f * cos(angle2)); fanVertices.push_back(0.5f * sin(angle2)); fanVertices.push_back(0.0f);
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

    // Draw arrows
    if (currentScreen != WATCH_SCREEN_CLOCK) {
        renderQuad(shader, arrowTexture, -0.14f * s, 0.0f, 0.04f * s, 0.04f * s, screenMatrix, true);
    }
    if (currentScreen != WATCH_SCREEN_BATTERY) {
        renderQuad(shader, arrowTexture, 0.14f * s, 0.0f, 0.04f * s, 0.04f * s, screenMatrix, false);
    }

    // Draw screen-specific content
    switch (currentScreen) {
        case WATCH_SCREEN_CLOCK:
            renderClockScreen(shader, screenMatrix);
            break;
        case WATCH_SCREEN_HEART_RATE:
            renderHeartRateScreen(shader, screenMatrix, currentTime);
            break;
        case WATCH_SCREEN_BATTERY:
            renderBatteryScreen(shader, screenMatrix);
            break;
    }
}

void Watch::renderClockScreen(unsigned int shader, const glm::mat4& parentModel) const {
    float s = contentScale;
    float scale = 0.045f * s;
    float totalWidth = (6 * 0.6f + 2 * 0.3f) * scale;
    digitRenderer->drawTime(hours, minutes, seconds, -totalWidth/2.0f, -0.02f * s, scale, glm::vec3(0.1f), shader, parentModel);
}

void Watch::renderHeartRateScreen(unsigned int shader, const glm::mat4& parentModel, double currentTime) const {
    float s = contentScale;

    // ECG
    renderECG(shader, 0.0f, -0.05f * s, 0.22f * s, 0.08f * s, parentModel);

    // BPM number
    float scale = 0.035f * s;
    digitRenderer->drawNumber(heartRate, -0.06f * s, 0.08f * s, scale, glm::vec3(0.8f, 0.0f, 0.0f), shader, parentModel);

    // Warning if heart rate too high
    if (heartRate > 200) {
        renderQuad(shader, warningTexture, 0.0f, 0.0f, 0.3f * s, 0.3f * s, parentModel);
    }
}

void Watch::renderBatteryScreen(unsigned int shader, const glm::mat4& parentModel) const {
    float s = contentScale;

    // Battery icon
    renderQuad(shader, batteryTexture, 0.0f, 0.0f, 0.16f * s, 0.09f * s, parentModel);

    // Battery bar
    float barWidth = 0.13f * s * (batteryPercent / 100.0f);
    glm::vec3 barColor(0.0f, 0.8f, 0.0f);
    if (batteryPercent < 20) barColor = glm::vec3(0.9f, 0.8f, 0.0f);
    if (batteryPercent < 10) barColor = glm::vec3(0.9f, 0.0f, 0.0f);

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

    // Percentage text
    float scale = 0.035f * s;
    digitRenderer->drawNumber(batteryPercent, -0.025f * s, 0.07f * s, scale, glm::vec3(0.1f), shader, parentModel);
    digitRenderer->drawPercent(0.04f * s, 0.07f * s, scale, glm::vec3(0.1f), shader, parentModel);
}

void Watch::renderQuad(unsigned int shader, unsigned int texture, float x, float y, float w, float h, const glm::mat4& parentModel, bool flipX) const {
    glm::mat4 model = parentModel;
    model = glm::translate(model, glm::vec3(x, y, 0.01f));
    model = glm::scale(model, glm::vec3(w, h, 1.0f));
    if (flipX) model = glm::scale(model, glm::vec3(-1.0f, 1.0f, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader, "uM"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shader, "uUseTexture"), 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shader, "uTexture"), 0);
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

void Watch::renderECG(unsigned int shader, float x, float y, float w, float h, const glm::mat4& parentModel) const {
    float texScale = 2.0f + ((heartRate - 60.0f) / 150.0f) * 2.0f;
    texScale = std::max(1.5f, std::min(4.0f, texScale));
    float heightScale = 1.0f + ((heartRate - 70.0f) / 150.0f) * 0.5f;
    heightScale = std::max(1.0f, std::min(1.5f, heightScale));

    glm::mat4 model = parentModel;
    model = glm::translate(model, glm::vec3(x, y, 0.01f));
    model = glm::scale(model, glm::vec3(w, h * heightScale, 1.0f));

    glUniformMatrix4fv(glGetUniformLocation(shader, "uM"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shader, "uUseTexture"), 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ecgTexture);
    glUniform1i(glGetUniformLocation(shader, "uTexture"), 0);
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kD"), 1, glm::value_ptr(glm::vec3(0.0f, 0.8f, 0.0f)));
    glUniform3fv(glGetUniformLocation(shader, "uMaterial.kA"), 1, glm::value_ptr(glm::vec3(0.0f, 0.5f, 0.0f)));

    float u0 = ecgScrollOffset;
    float u1 = ecgScrollOffset + texScale;

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

void Watch::nextScreen() {
    if (currentScreen == WATCH_SCREEN_CLOCK) currentScreen = WATCH_SCREEN_HEART_RATE;
    else if (currentScreen == WATCH_SCREEN_HEART_RATE) currentScreen = WATCH_SCREEN_BATTERY;
}

void Watch::prevScreen() {
    if (currentScreen == WATCH_SCREEN_HEART_RATE) currentScreen = WATCH_SCREEN_CLOCK;
    else if (currentScreen == WATCH_SCREEN_BATTERY) currentScreen = WATCH_SCREEN_HEART_RATE;
}

glm::vec3 Watch::getScreenPosition(const glm::mat4& handMatrix) const {
    glm::mat4 watchM = handMatrix;
    watchM = glm::translate(watchM, watchOffset);
    return glm::vec3(watchM * glm::vec4(0.0f, 0.1f, -0.02f, 1.0f));
}

void Watch::updateArrowPositions(const glm::mat4& view, const glm::mat4& proj, int screenWidth, int screenHeight) {
    // This would need access to the screen matrix - simplified for now
}
