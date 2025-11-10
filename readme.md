## Computer Graphics — Museum Showcase

This repository is a small Computer Graphics project that renders a simple museum-like room and places a set of imported 3D objects (tables and props) under a movable ceiling light. It was built as a learning / demo project to explore model import, material handling, normal mapping, and omnidirectional shadowing in OpenGL.

## What this project demonstrates
- Model import with Assimp (glTF support): meshes, UVs, and textures are imported and converted into the program's mesh/texture structures.
- Texture handling with stb_image and safe fallbacks for missing maps (solid-color 1x1 textures).
- Vertex layout convention: position (vec3), normal (vec3), uv (vec2); tangents are computed in the mesh builder so normal mapping works.
- Normal mapping (TBN-space) in the main shader.
- Point-light shadows using a depth cubemap (6-face depth pass) so a single ceiling bulb casts omnidirectional soft shadows.
- Scene composition helpers: source-space AABB computation for imported models, automatic centering and uniform scaling of props to fit tabletop footprints.
- Runtime interaction: move the ceiling light at runtime to inspect shadowing behavior.

## Technologies
- C++17
- OpenGL (GLEW + GLFW)
- glm for math
- Assimp for model import (glTF, OBJ, etc.)
- stb_image for image loading
- CMake based build system

## How to build and run
1. Install prerequisites: a C++ toolchain, CMake, and the libraries listed above (your package manager can usually provide them).
2. From the project root:

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
./main
```

The program will open a window and render the room, four tables, and four props. The console prints the current light position while you move it.

## Controls
- Camera: typical FPS-style keys (W/A/S/D, mouse to look) — see `main.cpp` for exact bindings.
- Move ceiling light (affects shadows):
	- Arrow keys: move in X/Z (left/right/forward/back)
	- , (comma): lower Y
	- . (period): raise Y

Tip: moving the light interactively is useful to inspect shadow behavior and tune bias/softness.

## Where to look in the code
- `main.cpp` — application entry, scene setup, render loop, and runtime controls.
- `include/AssimpLoader.hpp`, `src/AssimpLoader.cpp` — model import and creation of Renderable objects (mesh + textures + source AABB).
- `src/Shader.cpp` / `shaders/` — vertex/fragment shaders, including depth-cubemap depth shader and the main lighting shader.
- `include/ShadowCubemap.hpp`, `src/ShadowCubemap.cpp` — helper that allocates the depth cubemap and manages the 6-face depth pass.
- `src/Lightbulb.cpp`, `include/PointLight.hpp` — visual representation of the bulb and point-light uniform upload / setter.

## Credits & license
The project pulls together several well-known, permissively licensed libraries: Assimp, stb_image, glm, GLFW. See `third_party/` for bundled headers and their respective licenses.