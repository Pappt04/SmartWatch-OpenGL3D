#include "../Header/Sun.h"
#include "../Header/Util.h"
#include <glm/gtc/matrix_transform.hpp>

Sun::Sun()
    : meshReady(false),
      texture(0),
      position(-20.0f, 50.0f, -70.0f),
      scale(10.0f),
      ambient(0.35f, 0.35f, 0.35f),
      diffuse(1.3f, 1.25f, 1.15f),
      specular(1.2f, 1.2f, 1.1f) {
}

Sun::~Sun() {
    if (meshReady) sunMesh.cleanup();
}

void Sun::init(const char* texturePath) {
    sunMesh = Geometry::createSphere(32, 32);
    meshReady = true;
    texture = loadImageToTexture(texturePath);
}

void Sun::render(const ShaderUniforms& uniforms) const {
    if (!meshReady) return;

    uniforms.setFog(false);

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::scale(transform, glm::vec3(scale));
    uniforms.setModelMatrix(transform);

    uniforms.setMaterial(
        glm::vec3(0.0f),                // kD – zero
        glm::vec3(5.0f, 4.75f, 4.0f),  // kA –  brightness
        glm::vec3(0.0f),               // kS
        1.0f
    );

    uniforms.setTexture(true, texture);
    sunMesh.draw();
    uniforms.setTexture(false);
}
