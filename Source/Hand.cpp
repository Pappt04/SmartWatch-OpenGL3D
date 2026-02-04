#include "../Header/Hand.h"
#include <glm/gtc/matrix_transform.hpp>

Hand::Hand()
    : armModel(nullptr),
      skinKD(0.85f, 0.72f, 0.62f),
      skinKA(0.4f, 0.32f, 0.28f),
      skinKS(0.25f, 0.22f, 0.2f),
      skinShine(12.0f),
      armOffset(0.0f, 0.0f, 0.3f),
      armRotation(0.0f, 0.1f, 1.0f),
      armScale(0.02f) {
}

Hand::~Hand() {
    delete armModel;
}

void Hand::init(const char* armModelPath) {
    armModel = new Model(armModelPath);
}

void Hand::update(double deltaTime, const glm::vec3& cameraPos) {
    controller.update(deltaTime, cameraPos);
}

void Hand::render(const ShaderUniforms& uniforms) const {
    if (!armModel) return;

    glm::mat4 armTransform = getArmTransformMatrix();

    uniforms.setModelMatrix(armTransform);
    uniforms.setMaterial(skinKD, skinKA, skinKS, skinShine);
    uniforms.setTexture(false);

    armModel->draw();
}

void Hand::toggleViewingMode() {
    controller.toggleState();
}

bool Hand::isInViewingMode() const {
    return controller.isInViewingMode();
}

bool Hand::isInTransition() const {
    return controller.isInTransition();
}

glm::mat4 Hand::getTransformMatrix() const {
    return controller.getTransformMatrix();
}

glm::mat4 Hand::getArmTransformMatrix() const {
    glm::mat4 handM = controller.getTransformMatrix();
    glm::mat4 armTransform = handM;
    armTransform = glm::translate(armTransform, armOffset);
    armTransform = glm::rotate(armTransform, glm::radians(180.0f), armRotation);
    armTransform = glm::scale(armTransform, glm::vec3(armScale));
    return armTransform;
}
