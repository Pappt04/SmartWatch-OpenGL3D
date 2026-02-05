#include "../Header/HandController.h"
#include <algorithm>

HandController::HandController()
    : currentState(HAND_STATE_NORMAL),
      targetState(HAND_STATE_NORMAL),
      normalOffset(0.5f, -0.6f, -0.7f),       // Offset: right side, below eye level, in front
      viewingOffset(0.0f, 0.0f, -0.6f),       // Offset: centered in front of camera
      currentOffset(normalOffset),
      cameraPosition(0.0f),
      transitionProgress(0.0f),
      transitionSpeed(3.0f),                   // Smooth animation
      isTransitioning(false) {
}

void HandController::update(double deltaTime, const glm::vec3& camPos) {
    cameraPosition = camPos;

    if (isTransitioning) {
        transitionProgress += deltaTime * transitionSpeed;

        if (transitionProgress >= 1.0f) {
            transitionProgress = 1.0f;
            isTransitioning = false;
            currentState = targetState;
        }

        // Smooth interpolation (ease in-out)
        float t = transitionProgress;
        t = t * t * (3.0f - 2.0f * t);  // Smoothstep function

        // Interpolate offset
        if (targetState == HAND_STATE_VIEWING) {
            currentOffset = normalOffset + t * (viewingOffset - normalOffset);
        } else {
            currentOffset = viewingOffset + t * (normalOffset - viewingOffset);
        }
    }
}

void HandController::toggleState() {
    if (isTransitioning) return;  // Don't allow toggle during transition
    
    if (currentState == HAND_STATE_NORMAL) {
        targetState = HAND_STATE_VIEWING;
    } else {
        targetState = HAND_STATE_NORMAL;
    }
    
    isTransitioning = true;
    transitionProgress = 0.0f;
}

glm::mat4 HandController::getTransformMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    // Position relative to camera
    model = glm::translate(model, cameraPosition + currentOffset);

    // Rotate hand when in viewing mode
    if (currentState == HAND_STATE_VIEWING || isTransitioning) {
        float t = 0.0f;
        if (isTransitioning && targetState == HAND_STATE_VIEWING) {
            t = transitionProgress;
        } else if (isTransitioning && targetState == HAND_STATE_NORMAL) {
            t = 1.0f - transitionProgress;
        } else if (currentState == HAND_STATE_VIEWING) {
            t = 1.0f;
        }
        model = glm::rotate(model, t * glm::radians(90.0f),  glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, t * glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    return model;
}
