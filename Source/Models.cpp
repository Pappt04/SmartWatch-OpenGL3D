#include "../Header/Models.h"
#include "../Header/ShaderUniforms.h"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <cstdlib>

static std::unordered_map<std::string, glm::vec3> loadMTL(const std::string& mtlPath) {
    std::unordered_map<std::string, glm::vec3> colors;
    std::ifstream file(mtlPath);
    if (!file.is_open()) return colors;

    std::string currentName;
    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        if (start > 0) line = line.substr(start);

        if (line.size() >= 6 && line.substr(0, 6) == "newmtl") {
            currentName = line.substr(6);
            size_t s = currentName.find_first_not_of(" \t");
            if (s != std::string::npos) currentName = currentName.substr(s);
            size_t e = currentName.find_last_not_of(" \t\r\n");
            if (e != std::string::npos) currentName = currentName.substr(0, e + 1);
        } else if (line.size() >= 2 && line[0] == 'K' && line[1] == 'd' && !currentName.empty()) {
            float r = 0, g = 0, b = 0;
            sscanf(line.c_str() + 2, "%f %f %f", &r, &g, &b);
            colors[currentName] = glm::vec3(r, g, b);
        }
    }
    return colors;
}

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

Model::Model(const std::string& path) {
    loadModel(path);
}

Model::~Model() {
    for (auto& mesh : meshes) {
        mesh.cleanup();
    }
}

void Model::loadModel(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    directory = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : ".";

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

    size_t estLines = (size_t)fileSize / 40;
    std::vector<glm::vec3> positions;   positions.reserve(estLines);
    std::vector<glm::vec3> normals;     normals.reserve(estLines / 2);
    std::vector<glm::vec2> texCoords;   texCoords.reserve(estLines / 2);

    // Per-material vertex/index groups
    struct MaterialGroup {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::unordered_map<uint64_t, unsigned int> vertexMap;
    };
    std::unordered_map<std::string, MaterialGroup> groups;
    std::string currentMaterial = "_defaultMat";
    std::string mtlFile;

    const char* p   = buf.data();
    const char* end = p + fileSize;

    auto skipSpaces = [](const char*& ptr, const char* e) {
        while (ptr < e && (*ptr == ' ' || *ptr == '\t')) ++ptr;
    };

    auto nextLine = [](const char*& ptr, const char* e) {
        while (ptr < e && *ptr != '\n') ++ptr;
        if (ptr < e) ++ptr;
    };

    while (p < end) {
        if (*p == '#' || *p == '\n' || *p == '\r') { nextLine(p, end); continue; }

        if (*p == 'v') {
            ++p;
            if (p < end && *p == 'n') {
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
                ++p;
                skipSpaces(p, end);
                char* np;
                float u = strtof(p, &np); p = np;
                float v = strtof(p, &np); p = np;
                texCoords.push_back(glm::vec2(u, v));
                nextLine(p, end);
            }
            else if (p < end && (*p == ' ' || *p == '\t')) {
                skipSpaces(p, end);
                char* np;
                float x = strtof(p, &np); p = np;
                float y = strtof(p, &np); p = np;
                float z = strtof(p, &np); p = np;
                positions.push_back(glm::vec3(x, y, z));
                nextLine(p, end);
            }
            else {
                nextLine(p, end);
            }
        }
        else if (*p == 'f' && p + 1 < end && (p[1] == ' ' || p[1] == '\t')) {
            ++p;
            auto& grp = groups[currentMaterial];

            unsigned int faceIdx[32];
            int faceCount = 0;

            while (true) {
                skipSpaces(p, end);
                if (p >= end || *p == '\n' || *p == '\r') break;

                char* np;
                long posIdx  = strtol(p, &np, 10); p = np;
                long texIdx  = 0;
                long normIdx = 0;

                if (p < end && *p == '/') {
                    ++p;
                    if (p < end && *p != '/') {
                        texIdx = strtol(p, &np, 10); p = np;
                    }
                    if (p < end && *p == '/') {
                        ++p;
                        normIdx = strtol(p, &np, 10); p = np;
                    }
                }

                if (posIdx  < 0) posIdx  = (long)positions.size()  + posIdx  + 1;
                if (texIdx  < 0) texIdx  = (long)texCoords.size()  + texIdx  + 1;
                if (normIdx < 0) normIdx = (long)normals.size()    + normIdx + 1;

                uint64_t key = ((uint64_t)(unsigned)posIdx  << 40) |
                               ((uint64_t)(unsigned)texIdx  << 20) |
                                (uint64_t)(unsigned)normIdx;

                auto it = grp.vertexMap.find(key);
                if (it != grp.vertexMap.end()) {
                    faceIdx[faceCount++] = it->second;
                } else {
                    Vertex vertex;
                    vertex.position   = (posIdx > 0 && posIdx <= (long)positions.size())
                                        ? positions[posIdx - 1]  : glm::vec3(0.0f);
                    vertex.normal     = (normIdx > 0 && normIdx <= (long)normals.size())
                                        ? normals[normIdx - 1]   : glm::vec3(0.0f, 1.0f, 0.0f);
                    vertex.texCoords  = (texIdx > 0 && texIdx <= (long)texCoords.size())
                                        ? texCoords[texIdx - 1]  : glm::vec2(0.0f);

                    unsigned int idx = (unsigned int)grp.vertices.size();
                    grp.vertices.push_back(vertex);
                    grp.vertexMap[key] = idx;
                    faceIdx[faceCount++] = idx;
                }

                if (faceCount >= 32) break;
            }
            nextLine(p, end);

            for (int i = 1; i + 1 < faceCount; i++) {
                grp.indices.push_back(faceIdx[0]);
                grp.indices.push_back(faceIdx[i]);
                grp.indices.push_back(faceIdx[i + 1]);
            }
        }
        else if (*p == 'u' && p + 6 < end &&
                 p[1]=='s' && p[2]=='e' && p[3]=='m' && p[4]=='t' && p[5]=='l') {
            p += 6;
            skipSpaces(p, end);
            const char* nameStart = p;
            while (p < end && *p != '\n' && *p != '\r') ++p;
            currentMaterial = std::string(nameStart, p - nameStart);
            while (!currentMaterial.empty() &&
                   (currentMaterial.back() == ' ' || currentMaterial.back() == '\r' || currentMaterial.back() == '\t'))
                currentMaterial.pop_back();
            if (p < end && *p == '\r') ++p;
            if (p < end && *p == '\n') ++p;
        }
        else if (*p == 'm' && p + 6 < end &&
                 p[1]=='t' && p[2]=='l' && p[3]=='l' && p[4]=='i' && p[5]=='b') {
            p += 6;
            skipSpaces(p, end);
            const char* nameStart = p;
            while (p < end && *p != '\n' && *p != '\r') ++p;
            mtlFile = std::string(nameStart, p - nameStart);
            while (!mtlFile.empty() &&
                   (mtlFile.back() == ' ' || mtlFile.back() == '\r' || mtlFile.back() == '\t'))
                mtlFile.pop_back();
            if (p < end && *p == '\r') ++p;
            if (p < end && *p == '\n') ++p;
        }
        else {
            nextLine(p, end);
        }
    }

    std::unordered_map<std::string, glm::vec3> materialColors;
    if (!mtlFile.empty()) {
        materialColors = loadMTL(directory + "/" + mtlFile);
        if (materialColors.empty()) {
            size_t dotPos = path.rfind('.');
            if (dotPos != std::string::npos)
                materialColors = loadMTL(path.substr(0, dotPos) + ".mtl");
        }
    }

    for (auto& [name, grp] : groups) {
        if (grp.vertices.empty()) continue;
        Mesh mesh;
        mesh.vertices = std::move(grp.vertices);
        mesh.indices  = std::move(grp.indices);
        mesh.color    = materialColors.count(name) ? materialColors[name] : glm::vec3(0.8f);
        mesh.setupMesh();
        meshes.push_back(std::move(mesh));
    }
}

void Model::draw() const {
    for (const auto& mesh : meshes) {
        mesh.draw();
    }
}

void Model::drawWithMaterials(const ShaderUniforms& uniforms) const {
    for (const auto& mesh : meshes) {
        glm::vec3 kD = mesh.color;
        glm::vec3 kA = mesh.color * 0.65f;
        uniforms.setMaterial(kD, kA, glm::vec3(0.15f), 16.0f);
        uniforms.setTexture(false);
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

        float armLength = 1.0f;
        float armWidth = 0.18f;
        float armHeight = 0.14f;
        
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
            
            // Two triangles
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
                v.normal    = v.position;
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
