#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum HandState {
    HAND_STATE_NORMAL,    // Hand on right side
    HAND_STATE_VIEWING    // Hand in center (viewing watch)
};

class HandController {
private:
    HandState currentState;
    HandState targetState;

    glm::vec3 normalOffset;       // Offset from camera when on right side
    glm::vec3 viewingOffset;      // Offset from camera when viewing watch
    glm::vec3 currentOffset;
    glm::vec3 cameraPosition;     // Current camera position

    float transitionProgress;     // 0.0 to 1.0
    float transitionSpeed;        // Speed of transition animation

    bool isTransitioning;

public:
    HandController();

    // Update hand position (call every frame with camera position)
    void update(double deltaTime, const glm::vec3& camPos);

    // Toggle between states
    void toggleState();

    // Get current hand transformation matrix
    glm::mat4 getTransformMatrix() const;

    // Get current world position
    glm::vec3 getPosition() const { return cameraPosition + currentOffset; }

    // Check if in viewing mode
    bool isInViewingMode() const { return currentState == HAND_STATE_VIEWING; }

    // Check if currently transitioning
    bool isInTransition() const { return isTransitioning; }
};
