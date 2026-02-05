#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct ShaderUniforms;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
    glm::vec3 color = glm::vec3(0.8f);

    void setupMesh();
    void draw() const;
    void cleanup();
};

class Model {
private:
    std::vector<Mesh> meshes;
    std::string directory;
    
    void loadModel(const std::string& path);
    
public:
    Model(const std::string& path);
    ~Model();
    
    void draw() const;
    void drawWithMaterials(const ShaderUniforms& uniforms) const;
};

namespace Geometry {
    Mesh createGroundPlane(float width, float depth, int subdivisions = 10);
    
    Mesh createHandModel();
    
    Mesh createWatchScreen(float size, int segments = 32);

    Mesh createWatchBody(float diameter, float depth, int segments = 32);

    Mesh createRoadSegment(float width, float length);

    Mesh createSphere(int latSegments = 32, int lonSegments = 32);
}
