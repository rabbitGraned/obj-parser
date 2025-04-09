#include "obj_mesh_parser.h"

#include <iostream>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: parser.exe <input_file>\n";
        return 1;
    }

    std::string filePath = argv[1];

    auto warningCallback = [](const std::string& message, int lineNumber) {
        if (lineNumber != -1) {
            std::cerr << "Warning at line " << lineNumber << ": " << message << "\n";
        }
        else {
            std::cerr << "Warning: " << message << "\n";
        }
        };

    try {
        auto start = std::chrono::high_resolution_clock::now();
        Mesh mesh = parseOBJ(filePath, warningCallback);
        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "Vertices: " << mesh.vertices.size() << "\n";
        std::cout << "Polygons: " << mesh.polygons.size() << "\n";
        std::cout << "Materials: " << mesh.materials.size() << "\n";

        if (!mesh.materials.empty()) {
            std::cout << "Materials:\n";
            for (const auto& material : mesh.materials) {
                std::cout << "  Name: " << material.name << "\n";
                std::cout << "  Ambient: (" << material.ambient[0] << ", "
                    << material.ambient[1] << ", " << material.ambient[2] << ")\n";
                std::cout << "  Diffuse: (" << material.diffuse[0] << ", "
                    << material.diffuse[1] << ", " << material.diffuse[2] << ")\n";
                std::cout << "  Specular: (" << material.specular[0] << ", "
                    << material.specular[1] << ", " << material.specular[2] << ")\n";
                std::cout << "  Shininess: " << material.shininess << "\n";
                if (!material.textureFile.empty()) {
                    std::cout << "  Texture File Path: " << material.textureFile << " (not processed)\n";
                }
            }
        }

        if (!mesh.groups.empty()) {
            std::cout << "Groups:\n";
            for (const auto& group : mesh.groups) {
                std::cout << "  Name: " << group.name << "\n";
                std::cout << "  Start Index: " << group.startIndex << "\n";
                std::cout << "  Polygon Count: " << group.count << "\n";
                if (group.materialIndex != -1) {
                    std::cout << "  Material: " << mesh.materials[group.materialIndex].name << "\n";
                }
                else {
                    std::cout << "  Material: None\n";
                }
            }
        }
        else {
            std::cout << "Groups: " << mesh.groups.size() << "\n";
        }

        std::cout << "Parsing time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << " ms\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}