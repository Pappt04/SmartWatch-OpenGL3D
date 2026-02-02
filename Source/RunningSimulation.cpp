#include "../Header/RunningSimulation.h"

RunningSimulation::RunningSimulation(float segmentLength, int numSegments)
    : isRunning(false),
      speed(5.0f),
      segmentLength(segmentLength),
      numSegments(numSegments) {
    reset();
}

void RunningSimulation::reset() {
    segmentPositions.clear();
    
    // Initialize segments in a line
    for (int i = 0; i < numSegments; i++) {
        segmentPositions.push_back(-i * segmentLength);
    }
}

void RunningSimulation::update(double deltaTime, bool running) {
    isRunning = running;
    
    if (!isRunning) return;
    
    // Move all segments toward camera
    float movement = speed * deltaTime;
    
    for (float& pos : segmentPositions) {
        pos += movement;
    }
    
    // Recycle segments that have passed behind the camera
    // Find the segment furthest back
    float minPos = segmentPositions[0];
    int minIdx = 0;
    
    for (int i = 1; i < segmentPositions.size(); i++) {
        if (segmentPositions[i] < minPos) {
            minPos = segmentPositions[i];
            minIdx = i;
        }
    }
    
    // If any segment is too far ahead (passed camera), recycle the furthest back segment
    for (int i = 0; i < segmentPositions.size(); i++) {
        if (segmentPositions[i] > 5.0f) {  // Passed camera
            // Move the furthest back segment to the front
            segmentPositions[minIdx] = minPos - segmentLength;
            break;
        }
    }
}
