# OBJ Parser
#### Simple C++ OBJ file parser with MTL support.
---
![image](https://github.com/user-attachments/assets/6385e0c3-80c6-4b39-864d-7442e5715a42)

Scene: «Oasis re-draw» by rabbitGraned


## About:

The primary implementation of OBJ and MTL parsing with basic attributes.

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
## Modelsviewer:

The version in the Legacy branch has two variations: `modelsviewer` and `modelsviewer_dev` – 'dev' is compiled based on obj_mesh_parser.cpp from the dev branch, and the 'modelsviewer' is directly legacy.

The modelsviewer is only available on Windows.

## License:

MIT License © rabbitGraned R&D Lab
