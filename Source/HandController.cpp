#include "../Header/HandController.h"
#include <algorithm>

HandController::HandController()
    : currentState(HAND_STATE_NORMAL),
      targetState(HAND_STATE_NORMAL),
      normalOffset(0.5f, -0.6f, -0.7f),
      viewingOffset(0.0f, 0.0f, -0.6f),
      currentOffset(normalOffset),
      cameraPosition(0.0f),
      transitionProgress(0.0f),
      transitionSpeed(3.0f),           
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

        float t = transitionProgress;
        t = t * t * (3.0f - 2.0f * t);

        if (targetState == HAND_STATE_VIEWING) {
            currentOffset = normalOffset + t * (viewingOffset - normalOffset);
        } else {
            currentOffset = viewingOffset + t * (normalOffset - viewingOffset);
        }
    }
}

void HandController::toggleState() {
    if (isTransitioning) return;
    
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
    model = glm::translate(model, cameraPosition + currentOffset);

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
