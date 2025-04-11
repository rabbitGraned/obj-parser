/**
 * MAIN-LOGIC-FILE:     obj_parser.cpp
 * HEADER-FILE:         obj_mesh_parser.h
 * 
 * This file implements the main logic for parsing OBJ files.
 *
 * This parser handles vertex positions, texture coordinates, normals,
 * faces (including triangulation), materials (through MTL files),
 * groups, and smoothing groups. It also includes functionality for
 * calculating normals if they are not provided in the OBJ file.
 * --
 * 
 * Source code is provided by "rabbitGraned R&D Lab"
 * Licensed after MIT License
 * 
 * Clang/GCC:                       clang++ obj_parser.cpp obj_mesh_parser.cpp -o obj_parser.exe -O2 -std=c++20
 * MSVC (or F5 in Visual Studio):   cl /EHsc /O2 /std:c++20 obj_parser.cpp obj_mesh_parser.cpp /Fe:obj_parser
 */

#include "obj_mesh_parser.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <cmath>
#include <filesystem>
#include <chrono>
#include <thread>

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

void triangulateQuad(const std::vector<int>& quad, std::vector<Polygon>& triangles) {
    triangles.push_back(Polygon{ {quad[0], quad[1], quad[2]}, {-1, -1, -1}, {-1, -1, -1}, -1 });
    triangles.push_back(Polygon{ {quad[0], quad[2], quad[3]}, {-1, -1, -1}, {-1, -1, -1}, -1 });
}

bool isPolygonValid(const std::vector<int>& polygon, const std::vector<std::array<float, 3>>& positions) {
    for (int index : polygon) {
        if (index < 0 || index >= positions.size()) {
            return false;
        }
    }
    return true;
}

/*
    EXTENDED_POLYGON_VALIDATION

    This functionality needs to be improved.
    When enabling it, it is also worth considering the output
    of possible errors and limitations on this output.
*/

#ifdef EXTENDED_POLYGON_VALIDATION
bool isPolygonValid(const std::vector<int>& polygon, const std::vector<std::array<float, 3>>& positions) {
    if (polygon.size() < 3) return false;
    for (size_t i = 0; i < polygon.size(); ++i) {
        int a = polygon[i];
        int b = polygon[(i + 1) % polygon.size()];
        int c = polygon[(i + 2) % polygon.size()];
        if (a == b || b == c || a == c) return false;
        if (a < 0 || a >= positions.size()) return false;
        if (b < 0 || b >= positions.size()) return false;
        if (c < 0 || c >= positions.size()) return false;
        float ax = positions[a][0], ay = positions[a][1];
        float bx = positions[b][0], by = positions[b][1];
        float cx = positions[c][0], cy = positions[c][1];
        float area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);
        if (std::abs(area) < 1e-6) return false;
    }
    return true;
}
#endif //EXTENDED_POLYGON_VALIDATION

void safeTriangulatePolygon(const std::vector<int>& polygon,
    const std::vector<std::array<float, 3>>& positions,
    std::vector<Polygon>& triangles,
    WarningCallback warningCallback,
    int lineNumber) {
    if (!isPolygonValid(polygon, positions)) {
        warningCallback("Invalid polygon indices.", lineNumber);
        return;
    }
    if (polygon.size() < 3) {
        warningCallback("Invalid polygon: less than 3 vertices.", lineNumber);
        return;
    }
    if (polygon.size() == 3) {
        triangles.push_back(Polygon{ {polygon[0], polygon[1], polygon[2]}, {-1, -1, -1}, {-1, -1, -1}, -1 });
        return;
    }
    if (polygon.size() == 4) {
        triangulateQuad(polygon, triangles);
        return;
    }
    auto start = std::chrono::high_resolution_clock::now();
    try {
        std::vector<int> remainingPolygon = polygon;
        while (remainingPolygon.size() > 3) {
            bool progressMade = false;
            for (size_t i = 0; i < remainingPolygon.size(); ++i) {

                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                if (elapsed > 3000) {
                    throw std::runtime_error("Triangulation timeout.");
                }

                int a = remainingPolygon[i];
                int b = remainingPolygon[(i + 1) % remainingPolygon.size()];
                int c = remainingPolygon[(i + 2) % remainingPolygon.size()];
                bool isEar = true;
                for (size_t j = 0; j < remainingPolygon.size(); ++j) {
                    if (j == i || j == (i + 1) % remainingPolygon.size() || j == (i + 2) % remainingPolygon.size()) {
                        continue;
                    }
                    int p = remainingPolygon[j];
                    float ax = positions[a][0], ay = positions[a][1];
                    float bx = positions[b][0], by = positions[b][1];
                    float cx = positions[c][0], cy = positions[c][1];
                    float px = positions[p][0], py = positions[p][1];

                    float area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);
                    float epsilon = 1e-9f;
                    if (area < -epsilon) {
                        isEar = false;
                        break;
                    }
                }
                if (isEar) {
                    triangles.push_back(Polygon{ {a, b, c}, {-1, -1, -1}, {-1, -1, -1}, -1 });
                    remainingPolygon.erase(remainingPolygon.begin() + (i + 1));
                    progressMade = true;
                    break;
                }
            }
            if (!progressMade) {
                throw std::runtime_error("Unable to find ear for triangulation.");
            }
        }
        triangles.push_back(Polygon{ {remainingPolygon[0], remainingPolygon[1], remainingPolygon[2]},
                                     {-1, -1, -1}, {-1, -1, -1}, -1 });
    }
    catch (const std::exception& e) {
        warningCallback(std::string("Triangulation failed: ") + e.what(), lineNumber);

        int base = polygon[0];
        for (size_t i = 1; i < polygon.size() - 1; ++i) {
            triangles.push_back(Polygon{ {base, polygon[i], polygon[i + 1]}, {-1, -1, -1}, {-1, -1, -1}, -1 });
        }
    }
}

void calculateNormals(Mesh& mesh) {
    for (auto& polygon : mesh.polygons) {
        const auto& v0 = mesh.vertices[polygon.vertexIndices[0]];
        const auto& v1 = mesh.vertices[polygon.vertexIndices[1]];
        const auto& v2 = mesh.vertices[polygon.vertexIndices[2]];

        float edge1x = v1.x - v0.x;
        float edge1y = v1.y - v0.y;
        float edge1z = v1.z - v0.z;
        float edge2x = v2.x - v0.x;
        float edge2y = v2.y - v0.y;
        float edge2z = v2.z - v0.z;

        float nx = edge1y * edge2z - edge1z * edge2y;
        float ny = edge1z * edge2x - edge1x * edge2z;
        float nz = edge1x * edge2y - edge1y * edge2x;

        float length = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (length > 0) {
            nx /= length;
            ny /= length;
            nz /= length;
        }

        if (polygon.smoothing) {
            for (int i = 0; i < 3; ++i) {
                mesh.vertices[polygon.vertexIndices[i]].nx += nx;
                mesh.vertices[polygon.vertexIndices[i]].ny += ny;
                mesh.vertices[polygon.vertexIndices[i]].nz += nz;
            }
        }
        else {
            for (int i = 0; i < 3; ++i) {
                mesh.vertices[polygon.vertexIndices[i]].nx = nx;
                mesh.vertices[polygon.vertexIndices[i]].ny = ny;
                mesh.vertices[polygon.vertexIndices[i]].nz = nz;
            }
        }
    }

    for (auto& vertex : mesh.vertices) {
        float length = std::sqrt(vertex.nx * vertex.nx + vertex.ny * vertex.ny + vertex.nz * vertex.nz);
        if (length > 0) {
            vertex.nx /= length;
            vertex.ny /= length;
            vertex.nz /= length;
        }
    }
}

void parseMTL(const std::string& filePath, std::vector<Material>& materials, WarningCallback warningCallback) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        warningCallback("Failed to open MTL file: " + filePath, -1);
        return;
    }
    Material currentMaterial;
    std::string line;
    int lineNumber = 0;
    int maxWarnings = 1;
    int warningCount = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "newmtl") {
            if (!currentMaterial.name.empty()) {
                if (currentMaterial.diffuse[0] == 0.0f && currentMaterial.diffuse[1] == 0.0f && currentMaterial.diffuse[2] == 0.0f) {
                    if (warningCount < maxWarnings) {
                        warningCallback("Material '" + currentMaterial.name + "' is missing diffuse color.", lineNumber);
                        ++warningCount;
                    }
                }
                materials.push_back(currentMaterial);
            }
            currentMaterial = Material();
            iss >> currentMaterial.name;
        }
        else if (type == "Ka") {
            if (!(iss >> currentMaterial.ambient[0] >> currentMaterial.ambient[1] >> currentMaterial.ambient[2])) {
                if (warningCount < maxWarnings) {
                    warningCallback("Invalid ambient color in material '" + currentMaterial.name + "'.", lineNumber);
                    ++warningCount;
                }
            }
        }
        else if (type == "Kd") {
            if (!(iss >> currentMaterial.diffuse[0] >> currentMaterial.diffuse[1] >> currentMaterial.diffuse[2])) {
                if (warningCount < maxWarnings) {
                    warningCallback("Invalid diffuse color in material '" + currentMaterial.name + "'.", lineNumber);
                    ++warningCount;
                }
            }
        }
        else if (type == "Ks") {
            if (!(iss >> currentMaterial.specular[0] >> currentMaterial.specular[1] >> currentMaterial.specular[2])) {
                if (warningCount < maxWarnings) {
                    warningCallback("Invalid specular color in material '" + currentMaterial.name + "'.", lineNumber);
                    ++warningCount;
                }
            }
        }
        else if (type == "Ns") {
            if (!(iss >> currentMaterial.shininess)) {
                if (warningCount < maxWarnings) {
                    warningCallback("Invalid shininess value in material '" + currentMaterial.name + "'.", lineNumber);
                    ++warningCount;
                }
            }
        }
        else if (type == "map_Kd") {
            iss >> currentMaterial.textureFile;
            if (!currentMaterial.textureFile.empty() && !std::filesystem::exists(currentMaterial.textureFile)) {
                if (warningCount < maxWarnings) {
                    warningCallback("Texture file '" + currentMaterial.textureFile + "' not found.", lineNumber);
                    ++warningCount;
                }
            }
        }
    }
    if (!currentMaterial.name.empty()) {
        if (currentMaterial.diffuse[0] == 0.0f && currentMaterial.diffuse[1] == 0.0f && currentMaterial.diffuse[2] == 0.0f) {
            if (warningCount < maxWarnings) {
                warningCallback("Material '" + currentMaterial.name + "' is missing diffuse color.", lineNumber);
                ++warningCount;
            }
        }
        materials.push_back(currentMaterial);
    }
}

Mesh parseOBJ(const std::string& filePath, WarningCallback warningCallback) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    Mesh mesh;
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 2>> texCoords;
    std::vector<std::array<float, 3>> normals;

    std::unordered_map<std::string, int> materialMap;
    Group currentGroup;
    bool hasMaterials = false;

    positions.reserve(100000);
    texCoords.reserve(100000);
    normals.reserve(100000);
    mesh.vertices.reserve(100000);
    mesh.polygons.reserve(200000);

    std::string line;
    int lineNumber = 0;
    int maxWarnings = 1;
    int warningCount = 0;

    bool smoothingEnabled = true;

    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            if (!(iss >> x >> y >> z)) {
                if (warningCount < maxWarnings) {
                    warningCallback("Invalid vertex data.", lineNumber);
                    ++warningCount;
                }
                continue;
            }
            positions.push_back({ x, y, z });
        }
        else if (type == "vt") {
            float u, v;
            if (!(iss >> u >> v)) {
                if (warningCount < maxWarnings) {
                    warningCallback("Invalid texture coordinate data.", lineNumber);
                    ++warningCount;
                }
                continue;
            }
            texCoords.push_back({ u, v });
        }
        else if (type == "vn") {
            float nx, ny, nz;
            if (!(iss >> nx >> ny >> nz)) {
                if (warningCount < maxWarnings) {
                    warningCallback("Invalid normal data.", lineNumber);
                    ++warningCount;
                }
                continue;
            }
            normals.push_back({ nx, ny, nz });
        }
        else if (type == "f") {
            std::vector<int> vertexIndices, texCoordIndices, normalIndices;
            std::string faceData;
            while (iss >> faceData) {
                auto indices = split(faceData, '/');
                if (indices.size() < 1 || indices.size() > 3) {
                    if (warningCount < maxWarnings) {
                        warningCallback("Invalid face format in OBJ file.", lineNumber);
                        ++warningCount;
                    }
                    continue;
                }
                int vertexIndex = std::stoi(indices[0]) - 1;
                if (vertexIndex < 0 || vertexIndex >= positions.size()) {
                    if (warningCount < maxWarnings) {
                        warningCallback("Invalid vertex index in face definition.", lineNumber);
                        ++warningCount;
                    }
                    continue;
                }
                vertexIndices.push_back(vertexIndex);

                if (indices.size() > 1 && !indices[1].empty()) {
                    int texCoordIndex = std::stoi(indices[1]) - 1;
                    if (texCoordIndex < 0 || texCoordIndex >= texCoords.size()) {
                        texCoordIndex = -1;
                    }
                    texCoordIndices.push_back(texCoordIndex);
                }
                else {
                    texCoordIndices.push_back(-1);
                }

                if (indices.size() > 2 && !indices[2].empty()) {
                    int normalIndex = std::stoi(indices[2]) - 1;
                    if (normalIndex < 0 || normalIndex >= normals.size()) {
                        normalIndex = -1;
                    }
                    normalIndices.push_back(normalIndex);
                }
                else {
                    normalIndices.push_back(-1);
                }
            }

            std::vector<Polygon> triangles;
            safeTriangulatePolygon(vertexIndices, positions, triangles, warningCallback, lineNumber);
            for (auto& triangle : triangles) {
                triangle.smoothing = smoothingEnabled;
                mesh.polygons.push_back(triangle);
                ++currentGroup.count;
            }
        }
        else if (type == "o" || type == "g") {
            if (!currentGroup.name.empty() && currentGroup.count > 0) {
                mesh.groups.push_back(currentGroup);
            }
            currentGroup = { iss.str().substr(2), static_cast<size_t>(mesh.polygons.size()), 0, -1 };
        }
        else if (type == "usemtl") {
            std::string materialName;
            iss >> materialName;
            currentGroup.materialIndex = materialMap[materialName];
        }
        else if (type == "mtllib") {
            std::string mtlFile;
            iss >> mtlFile;

            std::filesystem::path objFilePath(filePath);
            std::filesystem::path mtlFilePath = objFilePath.parent_path() / mtlFile;

            parseMTL(mtlFilePath.string(), mesh.materials, warningCallback);

            for (size_t i = 0; i < mesh.materials.size(); ++i) {
                materialMap[mesh.materials[i].name] = static_cast<int>(i);
            }
        }
        else if (type == "s") {
            std::string smoothingValue;
            iss >> smoothingValue;
            if (smoothingValue == "off") {
                smoothingEnabled = false;
            }
            else {
                smoothingEnabled = true;
            }
            currentGroup.smoothing = smoothingEnabled;
        }
    }

    if (!currentGroup.name.empty() && currentGroup.count > 0) {
        mesh.groups.push_back(currentGroup);
    }

    for (size_t i = 0; i < positions.size(); ++i) {
        Vertex vertex;
        vertex.x = positions[i][0];
        vertex.y = positions[i][1];
        vertex.z = positions[i][2];
        vertex.nx = vertex.ny = vertex.nz = 0.0f;

        if (i < normals.size()) {
            vertex.nx = normals[i][0];
            vertex.ny = normals[i][1];
            vertex.nz = normals[i][2];
        }

        if (i < texCoords.size()) {
            vertex.u = texCoords[i][0];
            vertex.v = texCoords[i][1];
        }

        mesh.vertices.push_back(vertex);
    }

    if (normals.empty()) {
        calculateNormals(mesh);
    }

    return mesh;
}
