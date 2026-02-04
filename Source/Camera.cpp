#include "../Header/Camera.h"
#include <cmath>

Camera::Camera(glm::vec3 startPosition, float aspectRatio)
    : position(startPosition),
      worldUp(0.0f, 1.0f, 0.0f),
      yaw(-90.0f),
      pitch(0.0f),
      fov(50.0f),  // Slightly wider FOV for better scene coverage
      aspectRatio(aspectRatio),
      nearPlane(0.1f),
      farPlane(350.0f),  // Extended far plane to see distant buildings
      bobbingOffset(0.0f),
      bobbingSpeed(8.0f),
      bobbingAmount(0.08f) {  // Slightly more pronounced bobbing
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
    // Very limited vertical movement - mostly fixed street view
    if (position.y < 1.3f) position.y = 1.3f;
    if (position.y > 1.8f) position.y = 1.8f;
}

void Camera::moveForward(float speed) {
    position += front * speed;
}

void Camera::moveRight(float speed) {
    position += right * speed;
}

void Camera::moveUp(float speed) {
    position += worldUp * speed;
}

void Camera::rotate(float xoffset, float yoffset) {
    float sensitivity = 0.1f;
    yaw   += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    // When pitch overshoots a pole, reflect it and flip yaw so the camera
    // smoothly rolls over the top/bottom instead of getting stuck.
    if (pitch > 90.0f) {
        pitch = 180.0f - pitch;
        yaw  += 180.0f;
    }
    if (pitch < -90.0f) {
        pitch = -180.0f - pitch;
        yaw  += 180.0f;
    }

    updateCameraVectors();
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
