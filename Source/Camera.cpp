#include "../Header/Camera.h"
#include <cmath>

Camera::Camera(glm::vec3 startPosition, float aspectRatio)
    : position(startPosition),
      worldUp(0.0f, 1.0f, 0.0f),
      yaw(-90.0f),
      pitch(0.0f),
      fov(45.0f),
      aspectRatio(aspectRatio),
      nearPlane(0.1f),
      farPlane(100.0f),
      bobbingOffset(0.0f),
      bobbingSpeed(8.0f),
      bobbingAmount(0.05f) {
    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    // Calculate the new front vector
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    // Recalculate right and up vectors
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 actualPosition = position + glm::vec3(0.0f, bobbingOffset, 0.0f);
    return glm::lookAt(actualPosition, actualPosition + front, up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void Camera::moveVertical(float offset) {
    position.y += offset;
    // Clamp vertical position to reasonable bounds
    if (position.y < 0.5f) position.y = 0.5f;
    if (position.y > 5.0f) position.y = 5.0f;
}

void Camera::updateBobbing(double deltaTime, bool isRunning) {
    if (isRunning) {
        static float bobbingTime = 0.0f;
        bobbingTime += deltaTime * bobbingSpeed;
        bobbingOffset = sin(bobbingTime) * bobbingAmount;
    } else {
        // Smoothly return to zero
        bobbingOffset *= 0.95f;
        if (fabs(bobbingOffset) < 0.001f) {
            bobbingOffset = 0.0f;
        }
    }
}
