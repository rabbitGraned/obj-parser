# OBJ Parser
#### Simple C++ OBJ file parser with MTL support.
---
![image](https://github.com/user-attachments/assets/a370d046-e330-4a06-8656-41ab53ac5acc)

Model: Kiramman's Key by rabbitGraned, artwork link: https://www.artstation.com/artwork/y4Erd3



## About:
(For dev brunch):
- Parses `.obj` files into vertices, polygons, and groups
- Supports triangulation of quads
- Handles materials (`.mtl`) with texture and color information
- Calculates missing normals
- Provides warnings for invalid data
## Build and usage:

**Requirements**:
- C++20 compiler or higher;
- Modern STL implementation;
- For 'modelsviewer': OpenGL 1.0 via WinAPI.

**LLVM/Clang**:
```
clang++ obj_parser.cpp obj_mesh_parser.cpp -o obj_parser.exe -O2 -std=c++20
./obj_parser.exe your_model.obj
```

(Console)
Use the test app (obj_parser.cpp) for parsing:
obj_parser.* <path_to_model>
and utility for visualization:
modelsviewer.exe <path_to_model>

## Modelsviewer:

The version in the Legacy branch has two variations: `modelsviewer` and `modelsviewer_dev` – 'dev' is compiled based on obj_mesh_parser.cpp from the dev branch, and the 'modelsviewer' is directly legacy.

Enter 'N' to display the normals.

The `modelsviewer` is only available on Windows.

## License:

MIT License © rabbitGraned R&D Lab
