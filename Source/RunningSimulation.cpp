#include "../Header/RunningSimulation.h"
#include <iostream>
#include <algorithm>

RunningSimulation::RunningSimulation(float segmentLength, int numSegments)
    : isRunning(false),
      speed(8.0f),  // Faster running speed for larger segments
      segmentLength(segmentLength),
      numSegments(numSegments) {
    segmentPositions.clear(); // Ensure clear

    // Initialize segments in a line going backwards (-Z)
    for (int i = 0; i < numSegments; i++) {
        segmentPositions.push_back(-i * segmentLength);
    }
}

void RunningSimulation::reset() {
    segmentPositions.clear();
    for (int i = 0; i < numSegments; i++) {
        segmentPositions.push_back(-i * segmentLength);
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
        if (pos > 5.0f) { // Behind camera position with buffer
            // Move this segment to the back of the line
            pos = minZ - segmentLength;

            // Update minZ in case multiple segments wrap in one frame
            minZ = pos;
        }
    }
}
