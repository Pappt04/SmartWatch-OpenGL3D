#include "../Header/RunningSimulation.h"
#include <iostream>
#include <algorithm>

RunningSimulation::RunningSimulation(float segmentLength, int numSegments)
    : isRunning(false),
      speed(8.0f),
      segmentLength(segmentLength),
      numSegments(numSegments),
      buildingSpacing(4.0f) {
    reset();
}

void RunningSimulation::reset() {
    segmentPositions.clear();
    for (int i = 0; i < numSegments; i++) {
        segmentPositions.push_back(-i * segmentLength);
    }

    buildings.clear();
    float roadHalfWidth = 4.0f;
    int numBuildingsPerSide = 40;

    for (int i = 0; i < numBuildingsPerSide; i++) {
        float zPos = -2.0f - i * buildingSpacing;

        Building b1;
        b1.position = glm::vec3(-roadHalfWidth - 3.0f, 0.0f, zPos);
        b1.scale = 3.5f + (i % 3) * 0.5f;
        b1.type = i % 4;
        buildings.push_back(b1);

        Building b2;
        b2.position = glm::vec3(roadHalfWidth + 3.0f, 0.0f, zPos);
        b2.scale = 3.5f + ((i + 1) % 3) * 0.5f;
        b2.type = (i + 2) % 4;
        buildings.push_back(b2);
    }
}

void RunningSimulation::update(double deltaTime, bool running) {
    isRunning = running;
    
    if (!isRunning) return;
    
    float movement = speed * (float)deltaTime;
    
    for (float& pos : segmentPositions) {
        pos += movement;
    }
    
    float minZ = segmentPositions[0];
    for (float pos : segmentPositions) {
        if (pos < minZ) minZ = pos;
    }
    
    for (float& pos : segmentPositions) {
        if (pos > segmentLength + 5.0f) {
            pos = minZ - segmentLength;

            minZ = pos;
        }
    }

    // Move buildings
    for (auto& b : buildings) {
        b.position.z += movement;
    }

    // Find furthest back building to append after
    float minBuildingZ = 0.0f;
    if (!buildings.empty()) minBuildingZ = buildings[0].position.z;
    for (const auto& b : buildings) {
        if (b.position.z < minBuildingZ) minBuildingZ = b.position.z;
    }

    // Recycle buildings
    for (auto& b : buildings) {
        if (b.position.z > 5.0f) {
            b.position.z = minBuildingZ - buildingSpacing;
            minBuildingZ = b.position.z;
        }
    }
}
