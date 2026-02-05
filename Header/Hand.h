#pragma once
#include <glm/glm.hpp>
#include "Models.h"
#include "ShaderUniforms.h"
#include "HandController.h"

class Hand {
public:
    Hand();
    ~Hand();

    void init(const char* armModelPath);
    void update(double deltaTime, const glm::vec3& cameraPos);
    void render(const ShaderUniforms& uniforms) const;

    void toggleViewingMode();
    bool isInViewingMode() const;
    bool isInTransition() const;

    glm::mat4 getTransformMatrix() const;
    glm::mat4 getArmTransformMatrix() const;

private:
    Model* armModel;
    HandController controller;

    glm::vec3 skinKD, skinKA, skinKS;
    float skinShine;

    glm::vec3 armOffset;
    glm::vec3 armRotation;
    float armScale;
};
