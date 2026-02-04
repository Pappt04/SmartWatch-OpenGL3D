#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "Models.h"
#include "ShaderUniforms.h"
#include "RunningSimulation.h"

class Street {
public:
    Street();
    ~Street();

    void init(float roadWidth, float segmentLength, int numSegments);
    void update(double deltaTime, bool isRunning);
    void render(const ShaderUniforms& uniforms) const;

    const std::vector<float>& getSegmentPositions() const;

private:
    Mesh groundPlane;
    Mesh roadSegment;
    std::vector<Model*> buildingModels;
    RunningSimulation* simulation;

    unsigned int roadTexture;

    // Cached material properties
    struct {
        glm::vec3 groundKD, groundKA, groundKS;
        float groundShine;
        glm::vec3 roadKD, roadKA, roadKS;
        float roadShine;
        glm::vec3 buildingKD, buildingKA, buildingKS;
        float buildingShine;
    } materials;
};
