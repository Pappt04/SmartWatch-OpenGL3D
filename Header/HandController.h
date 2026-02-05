#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum HandState {
    HAND_STATE_NORMAL,
    HAND_STATE_VIEWING
};

class HandController {
private:
    HandState currentState;
    HandState targetState;

    glm::vec3 normalOffset;
    glm::vec3 viewingOffset;
    glm::vec3 currentOffset;
    glm::vec3 cameraPosition;

    float transitionProgress;
    float transitionSpeed;

    bool isTransitioning;

public:
    HandController();

    void update(double deltaTime, const glm::vec3& camPos);

    void toggleState();

    glm::mat4 getTransformMatrix() const;

    glm::vec3 getPosition() const { return cameraPosition + currentOffset; }

    bool isInViewingMode() const { return currentState == HAND_STATE_VIEWING; }

    bool isInTransition() const { return isTransitioning; }
};
