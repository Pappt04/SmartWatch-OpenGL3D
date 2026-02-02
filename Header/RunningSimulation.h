#pragma once
#include <vector>
#include <glm/glm.hpp>

class RunningSimulation {
public:
    struct Building {
        glm::vec3 position;
        float scale;
        int type;
    };

private:
    bool isRunning;
    float speed;
    float segmentLength;
    int numSegments;
    
    std::vector<float> segmentPositions;  // Z positions of road segments
    std::vector<Building> buildings;      // Moving buildings

public:
    RunningSimulation(float segmentLength = 10.0f, int numSegments = 5);
    
    void update(double deltaTime, bool running);
    void reset();
    
    // Get segment positions for rendering
    const std::vector<float>& getSegmentPositions() const { return segmentPositions; }
    const std::vector<Building>& getBuildings() const { return buildings; }
    
    bool getIsRunning() const { return isRunning; }
    float getSpeed() const { return speed; }
};
