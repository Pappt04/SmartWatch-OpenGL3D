#include "../Header/RunningSimulation.h"
#include <iostream>
#include <algorithm>

RunningSimulation::RunningSimulation(float segmentLength, int numSegments)
    : isRunning(false),
      speed(8.0f),  // Faster running speed for larger segments
      segmentLength(segmentLength),
      numSegments(numSegments) {
    reset();
}

void RunningSimulation::reset() {
    segmentPositions.clear();
    for (int i = 0; i < numSegments; i++) {
        segmentPositions.push_back(-i * segmentLength);
    }

    buildings.clear();
    float roadHalfWidth = 4.0f;
    float buildingSpacing = 10.0f;
    int numBuildingsPerSide = 18;

    for (int i = 0; i < numBuildingsPerSide; i++) {
        float zPos = -3.0f - i * buildingSpacing;

        // Left side buildings
        Building b1;
        b1.position = glm::vec3(-roadHalfWidth - 5.0f, 0.0f, zPos);
        b1.scale = 3.5f + (i % 3) * 1.0f;
        b1.type = i % 4;
        buildings.push_back(b1);

        // Right side buildings
        Building b2;
        b2.position = glm::vec3(roadHalfWidth + 5.0f, 0.0f, zPos - buildingSpacing * 0.5f);
        b2.scale = 3.0f + ((i + 1) % 4) * 0.9f;
        b2.type = (i + 2) % 4;
        buildings.push_back(b2);
    }
}

void RunningSimulation::update(double deltaTime, bool running) {
    isRunning = running;
    
    // Always update positions if running, or maybe even if not running to keep world consistent?
    // User spec: "While simulating running... surface moves towards us." -> Only when running.
    if (!isRunning) return;
    
    float movement = speed * (float)deltaTime;
    
    // Move all segments forward (+Z)
    for (float& pos : segmentPositions) {
        pos += movement;
    }
    
    // Find the furthest back position (minimum Z) to append to
    float minZ = segmentPositions[0];
    for (float pos : segmentPositions) {
        if (pos < minZ) minZ = pos;
    }
    
    // Check for segments that passed the camera (camera at Z=0)
    for (float& pos : segmentPositions) {
        if (pos > segmentLength + 5.0f) { // Behind camera position with buffer. Segment extends from pos-len to pos.
            // Move this segment to the back of the line
            pos = minZ - segmentLength;

            // Update minZ in case multiple segments wrap in one frame
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
            b.position.z = minBuildingZ - 10.0f; // 10.0f is buildingSpacing
            minBuildingZ = b.position.z;
        }
    }
}
