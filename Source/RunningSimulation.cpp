#include "../Header/RunningSimulation.h"
#include <iostream>
#include <algorithm>

RunningSimulation::RunningSimulation(float segmentLength, int numSegments)
    : isRunning(false),
      speed(5.0f),
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
    
    // Check for segments that passed the camera (e.g., Z > 5.0)
    for (float& pos : segmentPositions) {
        if (pos > 10.0f) { // Slightly behind camera (assuming camera at 5.0 looking at 0.0?)
            // Actually camera is at (0, 2, 5).
            // Road segments are at y=0.
            // If pos > 5.0, it is behind current camera Z.
            // Let's use 10.0 to be safe so it doesn't pop out visibly.
            
            // Move this segment to the back of the line
            pos = minZ - segmentLength;
            
            // Update minZ in case multiple segments wrap in one frame (unlikely but safe)
            minZ = pos;
        }
    }
}
