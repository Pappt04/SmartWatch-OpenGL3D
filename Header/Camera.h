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
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    
    void moveVertical(float offset);

    void moveForward(float speed);
    void moveRight(float speed);
    void moveUp(float speed);
    void rotate(float xoffset, float yoffset);

    void updateBobbing(double deltaTime, bool isRunning);
    float getBobbingOffset() const { return bobbingOffset; }
    
    glm::vec3 getPosition() const { return position + glm::vec3(0.0f, bobbingOffset, 0.0f); }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
    
    void setAspectRatio(float ratio) { aspectRatio = ratio; }
};
