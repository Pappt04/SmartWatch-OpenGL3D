#pragma once
#include <glm/glm.hpp>
#include "Models.h"
#include "ShaderUniforms.h"
#include "DigitRenderer.h"

enum WatchScreen {
    WATCH_SCREEN_CLOCK,
    WATCH_SCREEN_HEART_RATE,
    WATCH_SCREEN_BATTERY
};

class Watch {
public:
    Watch();
    ~Watch();

    void init();
    void update(double deltaTime, double currentTime, bool isRunning);
    void render(const ShaderUniforms& uniforms, const glm::mat4& handMatrix, unsigned int shader) const;
    void renderContent(unsigned int shader, const glm::mat4& screenMatrix, double currentTime) const;

    void nextScreen();
    void prevScreen();
    WatchScreen getCurrentScreen() const { return currentScreen; }

    glm::vec3 getScreenPosition(const glm::mat4& handMatrix) const;
    glm::vec2 getLeftArrowScreenPos() const { return leftArrowScreenPos; }
    glm::vec2 getRightArrowScreenPos() const { return rightArrowScreenPos; }
    void updateArrowPositions(const glm::mat4& view, const glm::mat4& proj, int screenWidth, int screenHeight);

    int getHeartRate() const { return heartRate; }
    int getBatteryPercent() const { return batteryPercent; }

private:
    Mesh watchBody;
    Mesh watchScreen;
    DigitRenderer* digitRenderer;

    WatchScreen currentScreen;

    // Time
    int hours, minutes, seconds;
    double lastTimeUpdate;

    // Heart rate
    int heartRate;
    double lastHeartUpdate;
    float ecgScrollOffset;

    // Battery
    int batteryPercent;
    double lastBatteryUpdate;

    // Textures
    unsigned int warningTexture;
    unsigned int ecgTexture;
    unsigned int batteryTexture;
    unsigned int arrowTexture;

    // Arrow positions for click detection
    mutable glm::vec2 leftArrowScreenPos;
    mutable glm::vec2 rightArrowScreenPos;

    // Watch position offset from hand
    glm::vec3 watchOffset;
    float contentScale;

    void renderClockScreen(unsigned int shader, const glm::mat4& parentModel) const;
    void renderHeartRateScreen(unsigned int shader, const glm::mat4& parentModel, double currentTime) const;
    void renderBatteryScreen(unsigned int shader, const glm::mat4& parentModel) const;
    void renderQuad(unsigned int shader, unsigned int texture, float x, float y, float w, float h, const glm::mat4& parentModel, bool flipX = false) const;
    void renderECG(unsigned int shader, float x, float y, float w, float h, const glm::mat4& parentModel) const;
};
