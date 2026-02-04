#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
    
    void setupMesh();
    void draw() const;
    void cleanup();
};

class Model {
private:
    std::vector<Mesh> meshes;
    std::string directory;
    
    void loadModel(const std::string& path);
    void processLine(const std::string& line, 
                    std::vector<glm::vec3>& positions,
                    std::vector<glm::vec3>& normals,
                    std::vector<glm::vec2>& texCoords,
                    std::vector<Vertex>& vertices,
                    std::vector<unsigned int>& indices);
    
public:
    Model(const std::string& path);
    ~Model();
    
    void draw() const;
};

// Simple geometry generators
namespace Geometry {
    // Create a ground plane
    Mesh createGroundPlane(float width, float depth, int subdivisions = 10);
    
    // Create a simple hand model
    Mesh createHandModel();
    
    // Create a circular watch screen
    Mesh createWatchScreen(float size, int segments = 32);

    // Create a cylindrical watch body
    Mesh createWatchBody(float diameter, float depth, int segments = 32);

    // Create a road segment
    Mesh createRoadSegment(float width, float length);

    // Create a UV sphere (radius 1.0, centred at origin)
    Mesh createSphere(int latSegments = 32, int lonSegments = 32);
}
