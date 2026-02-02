#include "../Header/HandController.h"
#include <algorithm>

HandController::HandController()
    : currentState(HAND_STATE_NORMAL),
      targetState(HAND_STATE_NORMAL),
      normalPosition(2.0f, 1.5f, -3.0f),      // Right side position
      viewingPosition(0.0f, 1.5f, -2.0f),     // Center position (in front of camera)
      currentPosition(normalPosition),
      transitionProgress(0.0f),
      transitionSpeed(3.0f),                   // Transition takes ~0.33 seconds
      isTransitioning(false) {
}

void HandController::update(double deltaTime) {
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
        
        // Interpolate position
        if (targetState == HAND_STATE_VIEWING) {
            currentPosition = normalPosition + t * (viewingPosition - normalPosition);
        } else {
            currentPosition = viewingPosition + t * (normalPosition - viewingPosition);
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
    model = glm::translate(model, currentPosition);
    
    // Rotate hand to face camera when in viewing mode
    if (currentState == HAND_STATE_VIEWING || isTransitioning) {
        float rotationAngle = 0.0f;
        if (isTransitioning && targetState == HAND_STATE_VIEWING) {
            rotationAngle = transitionProgress * glm::radians(90.0f);
        } else if (isTransitioning && targetState == HAND_STATE_NORMAL) {
            rotationAngle = (1.0f - transitionProgress) * glm::radians(90.0f);
        } else if (currentState == HAND_STATE_VIEWING) {
            rotationAngle = glm::radians(90.0f);
        }
        model = glm::rotate(model, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    return model;
}
