#ifndef OBJ_MESH_PARSER_H
#define OBJ_MESH_PARSER_H

#include <vector>
#include <string>
#include <functional>
#include <array>
#include <filesystem>

struct Vertex {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    float u = 0.0f, v = 0.0f;
};

struct Polygon {
    std::array<int, 3> vertexIndices = { -1, -1, -1 };
    std::array<int, 3> texCoordIndices = { -1, -1, -1 };
    std::array<int, 3> normalIndices = { -1, -1, -1 };
    int materialIndex = -1;
    bool smoothing = true;
};

struct Material {
    std::string name;
    float ambient[3] = { 0.2f, 0.2f, 0.2f };
    float diffuse[3] = { 0.8f, 0.8f, 0.8f };
    float specular[3] = { 1.0f, 1.0f, 1.0f };
    float shininess = 32.0f;
    std::string textureFile;
};

struct Group {
    std::string name;
    size_t startIndex = 0;
    size_t count = 0;
    int materialIndex = -1;
    bool smoothing = true;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Polygon> polygons;
    std::vector<Material> materials;
    std::vector<Group> groups;
};

using WarningCallback = std::function<void(const std::string& message, int lineNumber)>;
Mesh parseOBJ(const std::string& filePath, WarningCallback warningCallback);

/*  FOR GOOGLE TEST  */

//#ifdef COMMONTEST

void calculateNormals(Mesh& mesh);
void parseMTL(const std::string& filePath, std::vector<Material>& materials, WarningCallback warningCallback);
void triangulateQuad(const std::vector<int>& polygon, std::vector<Polygon>& triangles);

/*
void triangulatePolygon(const std::vector<int>& polygon, std::vector<Polygon>& triangles);
*/

//#endif // COMMONTEST

#endif
