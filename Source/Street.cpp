#include "../Header/Street.h"
#include "../Header/Util.h"
#include <glm/gtc/matrix_transform.hpp>

Street::Street()
    : simulation(nullptr), roadTexture(0) {
    // Initialize cached materials
    materials.groundKD = glm::vec3(0.2f, 0.6f, 0.15f);
    materials.groundKA = glm::vec3(0.1f, 0.25f, 0.08f);
    materials.groundKS = glm::vec3(0.02f);
    materials.groundShine = 2.0f;

    materials.roadKD = glm::vec3(0.6f);
    materials.roadKA = glm::vec3(0.25f);
    materials.roadKS = glm::vec3(0.1f);
    materials.roadShine = 4.0f;

    materials.buildingKD = glm::vec3(0.6f, 0.55f, 0.5f);
    materials.buildingKA = glm::vec3(0.4f, 0.38f, 0.35f);
    materials.buildingKS = glm::vec3(0.2f);
    materials.buildingShine = 16.0f;
}

Street::~Street() {
    groundPlane.cleanup();
    roadSegment.cleanup();
    for (auto* building : buildingModels) {
        delete building;
    }
    delete simulation;
}

void Street::init(float roadWidth, float segmentLength, int numSegments) {
    // Create geometry
    groundPlane = Geometry::createGroundPlane(200.0f, 400.0f, 50);
    roadSegment = Geometry::createRoadSegment(roadWidth, segmentLength);

    // Load textures
    roadTexture = loadImageToTexture("Resources/road.jpg");

    // Load building models
    buildingModels.push_back(new Model("Resources/ChonkyBuilding/chonky_buildingA.obj"));
    buildingModels.push_back(new Model("Resources/Skyscraper/skyscraperE.obj"));
    buildingModels.push_back(new Model("Resources/TallBuilding/tall_buildingC.obj"));
    buildingModels.push_back(new Model("Resources/Large Building/large_buildingE.obj"));

    // Initialize simulation
    simulation = new RunningSimulation(segmentLength, numSegments);
}

void Street::update(double deltaTime, bool isRunning) {
    if (simulation) {
        simulation->update(deltaTime, isRunning);
    }
}

void Street::render(const ShaderUniforms& uniforms) const {
    // Render ground plane
    glm::mat4 groundModel = glm::mat4(1.0f);
    uniforms.setModelMatrix(groundModel);
    uniforms.setMaterial(materials.groundKD, materials.groundKA, materials.groundKS, materials.groundShine);
    uniforms.setTexture(false);
    groundPlane.draw();

    // Render road segments
    uniforms.setMaterial(materials.roadKD, materials.roadKA, materials.roadKS, materials.roadShine);
    uniforms.setTexture(true, roadTexture);

    for (float zPos : simulation->getSegmentPositions()) {
        glm::mat4 segmentModel = glm::mat4(1.0f);
        segmentModel = glm::translate(segmentModel, glm::vec3(0.0f, 0.01f, zPos));
        uniforms.setModelMatrix(segmentModel);
        roadSegment.draw();
    }
    uniforms.setTexture(false);

    // Render buildings
    uniforms.setMaterial(materials.buildingKD, materials.buildingKA, materials.buildingKS, materials.buildingShine);

    const auto& buildings = simulation->getBuildings();
    for (const auto& b : buildings) {
        glm::mat4 bModel = glm::mat4(1.0f);
        bModel = glm::translate(bModel, b.position);
        bModel = glm::scale(bModel, glm::vec3(b.scale));
        uniforms.setModelMatrix(bModel);
        buildingModels[b.type]->draw();
    }
}

const std::vector<float>& Street::getSegmentPositions() const {
    return simulation->getSegmentPositions();
}
