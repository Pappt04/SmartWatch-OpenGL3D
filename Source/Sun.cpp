#include "../Header/Sun.h"
#include "../Header/Util.h"
#include <glm/gtc/matrix_transform.hpp>

Sun::Sun()
    : model(nullptr),
      texture(0),
      position(-30.0f, 80.0f, -150.0f),
      scale(25.0f),
      ambient(0.25f, 0.25f, 0.28f),
      diffuse(1.0f, 0.95f, 0.85f),
      specular(1.0f, 1.0f, 0.9f) {
}

Sun::~Sun() {
    delete model;
}

void Sun::init(const char* modelPath, const char* texturePath) {
    model = new Model(modelPath);
    texture = loadImageToTexture(texturePath);
}

void Sun::render(const ShaderUniforms& uniforms) const {
    if (!model) return;

    // Sun doesn't use fog
    uniforms.setFog(false);

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform = glm::scale(transform, glm::vec3(scale));

    uniforms.setModelMatrix(transform);

    // Bright emissive sun material
    uniforms.setMaterial(
        glm::vec3(1.0f, 0.9f, 0.7f),   // kD
        glm::vec3(1.0f, 0.95f, 0.8f),  // kA - high for glow
        glm::vec3(0.0f),               // kS
        1.0f                            // shine
    );

    uniforms.setTexture(true, texture);
    model->draw();
    uniforms.setTexture(false);
}
