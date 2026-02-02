#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    
    float yaw;
    float pitch;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    
    // Camera bobbing for running simulation
    float bobbingOffset;
    float bobbingSpeed;
    float bobbingAmount;
    
    void updateCameraVectors();
    
public:
    Camera(glm::vec3 startPosition, float aspectRatio);
    
    // Get view and projection matrices
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    
    // Camera movement (vertical only)
    void moveVertical(float offset);
    
    // Running simulation bobbing
    void updateBobbing(double deltaTime, bool isRunning);
    float getBobbingOffset() const { return bobbingOffset; }
    
    // Getters
    glm::vec3 getPosition() const { return position + glm::vec3(0.0f, bobbingOffset, 0.0f); }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
    
    // Update aspect ratio if window is resized
    void setAspectRatio(float ratio) { aspectRatio = ratio; }
};
