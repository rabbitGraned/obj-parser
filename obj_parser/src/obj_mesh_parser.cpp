/**
 * MAIN-LOGIC-FILE:     obj_parser.cpp
 * 
 * This file implements the main logic for parsing OBJ files.
 * Legacy version.
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

        for (int i = 0; i < 3; ++i) {
            mesh.vertices[polygon.vertexIndices[i]].nx += nx;
            mesh.vertices[polygon.vertexIndices[i]].ny += ny;
            mesh.vertices[polygon.vertexIndices[i]].nz += nz;
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

    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "newmtl") {
            if (!currentMaterial.name.empty()) {
                if (currentMaterial.diffuse[0] == 0.0f && currentMaterial.diffuse[1] == 0.0f && currentMaterial.diffuse[2] == 0.0f) {
                    warningCallback("Material '" + currentMaterial.name + "' is missing diffuse color.", lineNumber);
                }
                materials.push_back(currentMaterial);
            }
            currentMaterial = Material();
            iss >> currentMaterial.name;
        }
        else if (type == "Ka") {
            if (!(iss >> currentMaterial.ambient[0] >> currentMaterial.ambient[1] >> currentMaterial.ambient[2])) {
                warningCallback("Invalid ambient color in material '" + currentMaterial.name + "'.", lineNumber);
            }
        }
        else if (type == "Kd") {
            if (!(iss >> currentMaterial.diffuse[0] >> currentMaterial.diffuse[1] >> currentMaterial.diffuse[2])) {
                warningCallback("Invalid diffuse color in material '" + currentMaterial.name + "'.", lineNumber);
            }
        }
        else if (type == "Ks") {
            if (!(iss >> currentMaterial.specular[0] >> currentMaterial.specular[1] >> currentMaterial.specular[2])) {
                warningCallback("Invalid specular color in material '" + currentMaterial.name + "'.", lineNumber);
            }
        }
        else if (type == "Ns") {
            if (!(iss >> currentMaterial.shininess)) {
                warningCallback("Invalid shininess value in material '" + currentMaterial.name + "'.", lineNumber);
            }
        }
        else if (type == "map_Kd") {
            iss >> currentMaterial.textureFile;
            if (!currentMaterial.textureFile.empty() && !std::filesystem::exists(currentMaterial.textureFile)) {
                warningCallback("Texture file '" + currentMaterial.textureFile + "' not found.", lineNumber);
            }
        }
    }

    if (!currentMaterial.name.empty()) {
        if (currentMaterial.diffuse[0] == 0.0f && currentMaterial.diffuse[1] == 0.0f && currentMaterial.diffuse[2] == 0.0f) {
            warningCallback("Material '" + currentMaterial.name + "' is missing diffuse color.", lineNumber);
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

    positions.reserve(10000);
    texCoords.reserve(10000);
    normals.reserve(10000);
    mesh.vertices.reserve(10000);
    mesh.polygons.reserve(20000);

    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            if (!(iss >> x >> y >> z)) {
                warningCallback("Invalid vertex data.", lineNumber);
                continue;
            }
            positions.push_back({ x, y, z });
        }
        else if (type == "vt") {
            float u, v;
            if (!(iss >> u >> v)) {
                warningCallback("Invalid texture coordinate data.", lineNumber);
                continue;
            }
            texCoords.push_back({ u, v });
        }
        else if (type == "vn") {
            float nx, ny, nz;
            if (!(iss >> nx >> ny >> nz)) {
                warningCallback("Invalid normal data.", lineNumber);
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
                    warningCallback("Invalid face format in OBJ file.", lineNumber);
                    continue;
                }

                int vertexIndex = std::stoi(indices[0]) - 1;
                if (vertexIndex < 0 || vertexIndex >= positions.size()) {
                    warningCallback("Invalid vertex index in face definition.", lineNumber);
                    continue;
                }
                vertexIndices.push_back(vertexIndex);

                if (indices.size() > 1 && !indices[1].empty()) {
                    int texCoordIndex = std::stoi(indices[1]) - 1;
                    if (texCoordIndex < 0 || texCoordIndex >= texCoords.size()) {
                        warningCallback("Invalid texture coordinate index in face definition.", lineNumber);
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
                        warningCallback("Invalid normal index in face definition.", lineNumber);
                        normalIndex = -1;
                    }
                    normalIndices.push_back(normalIndex);
                }
                else {
                    normalIndices.push_back(-1);
                }
            }

            if (vertexIndices.size() == 3) {
                mesh.polygons.push_back(Polygon{
                    {vertexIndices[0], vertexIndices[1], vertexIndices[2]},
                    {texCoordIndices[0], texCoordIndices[1], texCoordIndices[2]},
                    {normalIndices[0], normalIndices[1], normalIndices[2]},
                    currentGroup.materialIndex
                    });
            }
            else if (vertexIndices.size() == 4) {
                std::vector<Polygon> triangles;
                triangulateQuad(vertexIndices, triangles);
                for (auto& triangle : triangles) {
                    mesh.polygons.push_back(triangle);
                }
            }
            else {
                warningCallback("Invalid polygon with more than 4 vertices.", lineNumber);
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
            parseMTL(mtlFile, mesh.materials, warningCallback);
            for (size_t i = 0; i < mesh.materials.size(); ++i) {
                materialMap[mesh.materials[i].name] = static_cast<int>(i);
            }
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