#include "../Header/Models.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// Mesh implementation
void Mesh::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    
    // Texture coordinate attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    
    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

// Model implementation
Model::Model(const std::string& path) {
    loadModel(path);
}

Model::~Model() {
    for (auto& mesh : meshes) {
        mesh.cleanup();
    }
}

void Model::loadModel(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open model file: " << path << std::endl;
        return;
    }
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    
    std::string line;
    while (std::getline(file, line)) {
        processLine(line, positions, normals, texCoords, vertices, indices);
    }
    
    file.close();
    
    // Create mesh from loaded data
    if (!vertices.empty()) {
        Mesh mesh;
        mesh.vertices = vertices;
        mesh.indices = indices;
        mesh.setupMesh();
        meshes.push_back(mesh);
    }
}

void Model::processLine(const std::string& line,
                       std::vector<glm::vec3>& positions,
                       std::vector<glm::vec3>& normals,
                       std::vector<glm::vec2>& texCoords,
                       std::vector<Vertex>& vertices,
                       std::vector<unsigned int>& indices) {
    if (line.empty() || line[0] == '#') return;
    
    std::istringstream iss(line);
    std::string prefix;
    iss >> prefix;
    
    if (prefix == "v") {
        // Vertex position
        glm::vec3 pos;
        iss >> pos.x >> pos.y >> pos.z;
        positions.push_back(pos);
    }
    else if (prefix == "vn") {
        // Vertex normal
        glm::vec3 normal;
        iss >> normal.x >> normal.y >> normal.z;
        normals.push_back(normal);
    }
    else if (prefix == "vt") {
        // Texture coordinate
        glm::vec2 tex;
        iss >> tex.x >> tex.y;
        texCoords.push_back(tex);
    }
    else if (prefix == "f") {
        // Face
        std::string vertexData;
        std::vector<unsigned int> faceIndices;
        
        while (iss >> vertexData) {
            std::replace(vertexData.begin(), vertexData.end(), '/', ' ');
            std::istringstream viss(vertexData);
            
            int posIdx = 0, texIdx = 0, normIdx = 0;
            viss >> posIdx >> texIdx >> normIdx;
            
            Vertex vertex;
            if (posIdx > 0 && posIdx <= positions.size()) {
                vertex.position = positions[posIdx - 1];
            }
            if (normIdx > 0 && normIdx <= normals.size()) {
                vertex.normal = normals[normIdx - 1];
            } else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);  // Default normal
            }
            if (texIdx > 0 && texIdx <= texCoords.size()) {
                vertex.texCoords = texCoords[texIdx - 1];
            } else {
                vertex.texCoords = glm::vec2(0.0f, 0.0f);  // Default tex coords
            }
            
            vertices.push_back(vertex);
            faceIndices.push_back(vertices.size() - 1);
        }
        
        // Triangulate face (assuming convex polygons)
        for (size_t i = 1; i + 1 < faceIndices.size(); i++) {
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[i]);
            indices.push_back(faceIndices[i + 1]);
        }
    }
}

void Model::draw() const {
    for (const auto& mesh : meshes) {
        mesh.draw();
    }
}

// Geometry generators
namespace Geometry {
    Mesh createGroundPlane(float width, float depth, int subdivisions) {
        Mesh mesh;
        
        float stepX = width / subdivisions;
        float stepZ = depth / subdivisions;
        
        // Generate vertices
        for (int z = 0; z <= subdivisions; z++) {
            for (int x = 0; x <= subdivisions; x++) {
                Vertex vertex;
                vertex.position = glm::vec3(
                    -width / 2.0f + x * stepX,
                    0.0f,
                    -depth / 2.0f + z * stepZ
                );
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vertex.texCoords = glm::vec2(
                    (float)x / subdivisions,
                    (float)z / subdivisions
                );
                mesh.vertices.push_back(vertex);
            }
        }
        
        // Generate indices
        for (int z = 0; z < subdivisions; z++) {
            for (int x = 0; x < subdivisions; x++) {
                int topLeft = z * (subdivisions + 1) + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * (subdivisions + 1) + x;
                int bottomRight = bottomLeft + 1;
                
                mesh.indices.push_back(topLeft);
                mesh.indices.push_back(bottomLeft);
                mesh.indices.push_back(topRight);
                
                mesh.indices.push_back(topRight);
                mesh.indices.push_back(bottomLeft);
                mesh.indices.push_back(bottomRight);
            }
        }
        
        mesh.setupMesh();
        return mesh;
    }
    
    Mesh createHandModel() {
        Mesh mesh;

        // Arm/wrist model - sized to match watch proportions
        float armLength = 1.0f;   // Standard arm length
        float armWidth = 0.18f;   // Standard width
        float armHeight = 0.14f;  // Standard height
        
        // Define 8 vertices of the box
        glm::vec3 vertices[8] = {
            glm::vec3(-armWidth/2, -armHeight/2, 0.0f),           // 0: back-bottom-left
            glm::vec3(armWidth/2, -armHeight/2, 0.0f),            // 1: back-bottom-right
            glm::vec3(armWidth/2, armHeight/2, 0.0f),             // 2: back-top-right
            glm::vec3(-armWidth/2, armHeight/2, 0.0f),            // 3: back-top-left
            glm::vec3(-armWidth/2, -armHeight/2, -armLength),     // 4: front-bottom-left
            glm::vec3(armWidth/2, -armHeight/2, -armLength),      // 5: front-bottom-right
            glm::vec3(armWidth/2, armHeight/2, -armLength),       // 6: front-top-right
            glm::vec3(-armWidth/2, armHeight/2, -armLength)       // 7: front-top-left
        };
        
        // Define faces with normals
        struct Face {
            int v[4];
            glm::vec3 normal;
        };
        
        Face faces[6] = {
            {{0, 1, 2, 3}, glm::vec3(0, 0, 1)},   // Back
            {{5, 4, 7, 6}, glm::vec3(0, 0, -1)},  // Front
            {{4, 0, 3, 7}, glm::vec3(-1, 0, 0)},  // Left
            {{1, 5, 6, 2}, glm::vec3(1, 0, 0)},   // Right
            {{3, 2, 6, 7}, glm::vec3(0, 1, 0)},   // Top
            {{4, 5, 1, 0}, glm::vec3(0, -1, 0)}   // Bottom
        };
        
        // Build mesh
        for (int f = 0; f < 6; f++) {
            int baseIdx = mesh.vertices.size();
            
            for (int i = 0; i < 4; i++) {
                Vertex v;
                v.position = vertices[faces[f].v[i]];
                v.normal = faces[f].normal;
                v.texCoords = glm::vec2((i % 2), (i / 2));
                mesh.vertices.push_back(v);
            }
            
            // Two triangles per face
            mesh.indices.push_back(baseIdx + 0);
            mesh.indices.push_back(baseIdx + 1);
            mesh.indices.push_back(baseIdx + 2);
            
            mesh.indices.push_back(baseIdx + 0);
            mesh.indices.push_back(baseIdx + 2);
            mesh.indices.push_back(baseIdx + 3);
        }
        
        mesh.setupMesh();
        return mesh;
    }
    
    Mesh createWatchScreen(float size, int segments) {
        Mesh mesh;

        float radius = size / 2.0f;

        // Create a circular disc facing forward (along +Z axis)
        // Center vertex
        Vertex center;
        center.position = glm::vec3(0.0f, 0.0f, 0.0f);
        center.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        center.texCoords = glm::vec2(0.5f, 0.5f);
        mesh.vertices.push_back(center);

        // Edge vertices
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / (float)segments * 2.0f * 3.14159265f;
            Vertex v;
            v.position = glm::vec3(radius * cos(angle), radius * sin(angle), 0.0f);
            v.normal = glm::vec3(0.0f, 0.0f, 1.0f);
            v.texCoords = glm::vec2(0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle));
            mesh.vertices.push_back(v);
        }

        // Create triangles (fan from center)
        for (int i = 1; i <= segments; i++) {
            mesh.indices.push_back(0);       // center
            mesh.indices.push_back(i);       // current edge
            mesh.indices.push_back(i + 1);   // next edge
        }

        mesh.setupMesh();
        return mesh;
    }
    
    Mesh createWatchBody(float diameter, float depth, int segments) {
        Mesh mesh;

        float radius = diameter / 2.0f;
        float hd = depth / 2.0f;

        // Cylindrical watch case
        // Back face (circle at -Z)
        int backCenterIdx = mesh.vertices.size();
        Vertex backCenter;
        backCenter.position = glm::vec3(0.0f, 0.0f, -hd);
        backCenter.normal = glm::vec3(0.0f, 0.0f, -1.0f);
        backCenter.texCoords = glm::vec2(0.5f, 0.5f);
        mesh.vertices.push_back(backCenter);

        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / (float)segments * 2.0f * 3.14159265f;
            Vertex v;
            v.position = glm::vec3(radius * cos(angle), radius * sin(angle), -hd);
            v.normal = glm::vec3(0.0f, 0.0f, -1.0f);
            v.texCoords = glm::vec2(0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle));
            mesh.vertices.push_back(v);
        }

        // Back face triangles
        for (int i = 1; i <= segments; i++) {
            mesh.indices.push_back(backCenterIdx);
            mesh.indices.push_back(backCenterIdx + i + 1);
            mesh.indices.push_back(backCenterIdx + i);
        }

        // Side wall (cylinder)
        int sideStartIdx = mesh.vertices.size();
        for (int i = 0; i <= segments; i++) {
            float angle = (float)i / (float)segments * 2.0f * 3.14159265f;
            float nx = cos(angle);
            float ny = sin(angle);

            // Back edge
            Vertex vBack;
            vBack.position = glm::vec3(radius * nx, radius * ny, -hd);
            vBack.normal = glm::vec3(nx, ny, 0.0f);
            vBack.texCoords = glm::vec2((float)i / segments, 0.0f);
            mesh.vertices.push_back(vBack);

            // Front edge
            Vertex vFront;
            vFront.position = glm::vec3(radius * nx, radius * ny, hd);
            vFront.normal = glm::vec3(nx, ny, 0.0f);
            vFront.texCoords = glm::vec2((float)i / segments, 1.0f);
            mesh.vertices.push_back(vFront);
        }

        // Side wall triangles
        for (int i = 0; i < segments; i++) {
            int idx = sideStartIdx + i * 2;
            mesh.indices.push_back(idx);
            mesh.indices.push_back(idx + 1);
            mesh.indices.push_back(idx + 3);

            mesh.indices.push_back(idx);
            mesh.indices.push_back(idx + 3);
            mesh.indices.push_back(idx + 2);
        }

        mesh.setupMesh();
        return mesh;
    }

    Mesh createRoadSegment(float width, float length) {
        Mesh mesh;
        
        float halfWidth = width / 2.0f;
        
        Vertex v0, v1, v2, v3;
        
        v0.position = glm::vec3(-halfWidth, 0.0f, 0.0f);
        v0.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v0.texCoords = glm::vec2(0.0f, 0.0f);
        
        v1.position = glm::vec3(halfWidth, 0.0f, 0.0f);
        v1.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v1.texCoords = glm::vec2(0.0f, 1.0f);
        
        v2.position = glm::vec3(halfWidth, 0.0f, -length);
        v2.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v2.texCoords = glm::vec2(1.0f, 1.0f);
        
        v3.position = glm::vec3(-halfWidth, 0.0f, -length);
        v3.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        v3.texCoords = glm::vec2(1.0f, 0.0f);
        
        mesh.vertices = {v0, v1, v2, v3};
        mesh.indices = {0, 1, 2, 0, 2, 3};
        
        mesh.setupMesh();
        return mesh;
    }
}
