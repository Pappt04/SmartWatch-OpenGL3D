#pragma once
#include <vector>
#include <glm/glm.hpp>

class RunningSimulation {
private:
    bool isRunning;
    float speed;
    float segmentLength;
    int numSegments;
    
    std::vector<float> segmentPositions;  // Z positions of road segments
    
public:
    RunningSimulation(float segmentLength = 10.0f, int numSegments = 5);
    
    void update(double deltaTime, bool running);
    void reset();
    
    // Get segment positions for rendering
    const std::vector<float>& getSegmentPositions() const { return segmentPositions; }
    
    bool getIsRunning() const { return isRunning; }
    float getSpeed() const { return speed; }
};
