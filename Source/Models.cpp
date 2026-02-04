#include "../Header/Models.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <cstdlib>

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
    // Read entire file into memory in one shot
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open model file: " << path << std::endl;
        return;
    }
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buf((size_t)fileSize);
    if (!file.read(buf.data(), fileSize)) {
        std::cerr << "Failed to read model file: " << path << std::endl;
        return;
    }
    file.close();

    // Heuristic reserves based on file size (avoids repeated reallocations)
    size_t estLines = (size_t)fileSize / 40;
    std::vector<glm::vec3> positions;   positions.reserve(estLines);
    std::vector<glm::vec3> normals;     normals.reserve(estLines / 2);
    std::vector<glm::vec2> texCoords;   texCoords.reserve(estLines / 2);
    std::vector<Vertex>    vertices;    vertices.reserve(estLines / 2);
    std::vector<unsigned int> indices;  indices.reserve(estLines * 3 / 2);

    // Dedup map: key = packed (posIdx, texIdx, normIdx), value = output vertex index
    std::unordered_map<uint64_t, unsigned int> vertexMap;
    vertexMap.reserve(estLines);

    const char* p   = buf.data();
    const char* end = p + fileSize;

    // Helper: skip whitespace (spaces/tabs only, not newlines)
    auto skipSpaces = [](const char*& ptr, const char* e) {
        while (ptr < e && (*ptr == ' ' || *ptr == '\t')) ++ptr;
    };

    // Helper: advance to next line
    auto nextLine = [](const char*& ptr, const char* e) {
        while (ptr < e && *ptr != '\n') ++ptr;
        if (ptr < e) ++ptr; // skip the '\n'
    };

    while (p < end) {
        // Skip blank lines and comments
        if (*p == '#' || *p == '\n' || *p == '\r') { nextLine(p, end); continue; }

        if (*p == 'v') {
            ++p;
            if (p < end && *p == 'n') {
                // "vn" — vertex normal
                ++p;
                skipSpaces(p, end);
                char* np;
                float x = strtof(p, &np); p = np;
                float y = strtof(p, &np); p = np;
                float z = strtof(p, &np); p = np;
                normals.push_back(glm::vec3(x, y, z));
                nextLine(p, end);
            }
            else if (p < end && *p == 't') {
                // "vt" — texture coordinate
                ++p;
                skipSpaces(p, end);
                char* np;
                float u = strtof(p, &np); p = np;
                float v = strtof(p, &np); p = np;
                texCoords.push_back(glm::vec2(u, v));
                nextLine(p, end);
            }
            else if (p < end && (*p == ' ' || *p == '\t')) {
                // "v " — vertex position
                skipSpaces(p, end);
                char* np;
                float x = strtof(p, &np); p = np;
                float y = strtof(p, &np); p = np;
                float z = strtof(p, &np); p = np;
                positions.push_back(glm::vec3(x, y, z));
                nextLine(p, end);
            }
            else {
                nextLine(p, end); // unknown "v..." token, skip
            }
        }
        else if (*p == 'f' && p + 1 < end && (p[1] == ' ' || p[1] == '\t')) {
            // "f" — face
            ++p; // skip 'f'

            unsigned int faceIdx[32]; // max vertices per face before triangulation
            int faceCount = 0;

            while (true) {
                skipSpaces(p, end);
                if (p >= end || *p == '\n' || *p == '\r') break;

                // Parse v/vt/vn — each component is optional after the first
                char* np;
                long posIdx  = strtol(p, &np, 10); p = np;
                long texIdx  = 0;
                long normIdx = 0;

                if (p < end && *p == '/') {
                    ++p; // skip first '/'
                    if (p < end && *p != '/') {
                        texIdx = strtol(p, &np, 10); p = np;
                    }
                    if (p < end && *p == '/') {
                        ++p; // skip second '/'
                        normIdx = strtol(p, &np, 10); p = np;
                    }
                }

                // Convert negative (relative) indices
                if (posIdx  < 0) posIdx  = (long)positions.size()  + posIdx  + 1;
                if (texIdx  < 0) texIdx  = (long)texCoords.size()  + texIdx  + 1;
                if (normIdx < 0) normIdx = (long)normals.size()    + normIdx + 1;

                // Pack into a single 64-bit key (20 bits each is enough for ~1 M entries)
                uint64_t key = ((uint64_t)(unsigned)posIdx  << 40) |
                               ((uint64_t)(unsigned)texIdx  << 20) |
                                (uint64_t)(unsigned)normIdx;

                auto it = vertexMap.find(key);
                if (it != vertexMap.end()) {
                    faceIdx[faceCount++] = it->second;
                } else {
                    Vertex vertex;
                    vertex.position   = (posIdx > 0 && posIdx <= (long)positions.size())
                                        ? positions[posIdx - 1]  : glm::vec3(0.0f);
                    vertex.normal     = (normIdx > 0 && normIdx <= (long)normals.size())
                                        ? normals[normIdx - 1]   : glm::vec3(0.0f, 1.0f, 0.0f);
                    vertex.texCoords  = (texIdx > 0 && texIdx <= (long)texCoords.size())
                                        ? texCoords[texIdx - 1]  : glm::vec2(0.0f);

                    unsigned int idx = (unsigned int)vertices.size();
                    vertices.push_back(vertex);
                    vertexMap[key] = idx;
                    faceIdx[faceCount++] = idx;
                }

                if (faceCount >= 32) break; // safety guard
            }
            nextLine(p, end);

            // Fan triangulation (convex polygons)
            for (int i = 1; i + 1 < faceCount; i++) {
                indices.push_back(faceIdx[0]);
                indices.push_back(faceIdx[i]);
                indices.push_back(faceIdx[i + 1]);
            }
        }
        else {
            nextLine(p, end); // skip any other line (mtllib, usemtl, o, s, …)
        }
    }

    if (!vertices.empty()) {
        Mesh mesh;
        mesh.vertices = std::move(vertices);
        mesh.indices  = std::move(indices);
        mesh.setupMesh();
        meshes.push_back(std::move(mesh));
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

    Mesh createSphere(int latSegs, int lonSegs) {
        Mesh mesh;

        for (int i = 0; i <= latSegs; i++) {
            float theta    = 3.14159265f * (float)i / (float)latSegs;
            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);

            for (int j = 0; j <= lonSegs; j++) {
                float phi    = 2.0f * 3.14159265f * (float)j / (float)lonSegs;
                float sinPhi = sinf(phi);
                float cosPhi = cosf(phi);

                Vertex v;
                v.position  = glm::vec3(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
                v.normal    = v.position;   // unit sphere: normal == position
                v.texCoords = glm::vec2((float)j / (float)lonSegs, (float)i / (float)latSegs);
                mesh.vertices.push_back(v);
            }
        }

        for (int i = 0; i < latSegs; i++) {
            for (int j = 0; j < lonSegs; j++) {
                int v1 = i * (lonSegs + 1) + j;
                int v2 = v1 + 1;
                int v3 = (i + 1) * (lonSegs + 1) + j;
                int v4 = v3 + 1;

                mesh.indices.push_back(v1);
                mesh.indices.push_back(v3);
                mesh.indices.push_back(v2);

                mesh.indices.push_back(v2);
                mesh.indices.push_back(v3);
                mesh.indices.push_back(v4);
            }
        }

        mesh.setupMesh();
        return mesh;
    }
}
