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
    
    glm::vec3 normalPosition;     // Position when on right side
    glm::vec3 viewingPosition;    // Position when in center
    glm::vec3 currentPosition;
    
    float transitionProgress;     // 0.0 to 1.0
    float transitionSpeed;        // Speed of transition animation
    
    bool isTransitioning;
    
public:
    HandController();
    
    // Update hand position (call every frame)
    void update(double deltaTime);
    
    // Toggle between states
    void toggleState();
    
    // Get current hand transformation matrix
    glm::mat4 getTransformMatrix() const;
    
    // Get current position
    glm::vec3 getPosition() const { return currentPosition; }
    
    // Check if in viewing mode
    bool isInViewingMode() const { return currentState == HAND_STATE_VIEWING; }
    
    // Check if currently transitioning
    bool isInTransition() const { return isTransitioning; }
};
