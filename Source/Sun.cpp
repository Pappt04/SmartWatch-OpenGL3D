#include "../Header/Sun.h"
#include "../Header/Util.h"
#include <glm/gtc/matrix_transform.hpp>

Sun::Sun()
    : meshReady(false),
      texture(0),
      position(-20.0f, 50.0f, -70.0f),
      scale(10.0f),
      ambient(0.22f, 0.22f, 0.22f),
      diffuse(1.0f, 0.95f, 0.85f),
      specular(1.0f, 1.0f, 0.9f) {
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

    // The light source is at the sun's own centre, so diffuse is zero on every
    // visible fragment (normal · lightDir ≤ 0).  All brightness comes from the
    // ambient term: light.kA * material.kA * texColor.
    // light.kA = (0.2, 0.2, 0.2), so material.kA must be ~5× to saturate.
    uniforms.setMaterial(
        glm::vec3(0.0f),                // kD – zero, not used anyway
        glm::vec3(5.0f, 4.75f, 4.0f),  // kA – drives emissive brightness
        glm::vec3(0.0f),               // kS
        1.0f
    );

    uniforms.setTexture(true, texture);
    sunMesh.draw();
    uniforms.setTexture(false);
}
