#pragma once
#include <glm/glm.hpp>
#include "Models.h"
#include "ShaderUniforms.h"

class Sun {
public:
    Sun();
    ~Sun();

    void init(const char* modelPath, const char* texturePath);
    void render(const ShaderUniforms& uniforms) const;

    glm::vec3 getPosition() const { return position; }
    glm::vec3 getAmbient() const { return ambient; }
    glm::vec3 getDiffuse() const { return diffuse; }
    glm::vec3 getSpecular() const { return specular; }

    void setPosition(const glm::vec3& pos) { position = pos; }

private:
    Model* model;
    unsigned int texture;
    glm::vec3 position;
    float scale;

    // Light properties
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};
