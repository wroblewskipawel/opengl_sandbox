# rupture

Project aims to provide custom game engine tailored specifically to games using tile maps as their world representation and rellying heavily on procedural content generation.  
At the current state of the project, there is OpenGL based renderer implemented, making use of instanced indirect drawing commands and bindless texturing as a means to enable efficient tile map rendering.

---

### Supported features
* glTF2.0 model import
* Image based lighting
* PBR shading

### In development
* Procedurall map generation with automatically derived geometric constraints based on provided tile meshes
* Custom ECS framework
* Skeletal animation system
* Quad-tree space partitioning
---
### Build instructions

Code has been tested on Ubuntu 20.04 with both GCC and Clang compilers, on a system with NVidia graphics card. Support for ```GL_ARB_bindless_texture``` and ```GL_ARB_shader_draw_parameters``` OpenGL extension is required.

* Clone the repository with recursive flag

```
    git clone --recursive https://github.com/wroblewskipawel/rupture.git
```
* Build using CMake and Ninja
```
    mkdir build
    cd build
    cmake .. -G "Ninja"
    cmake --build .
    cmake --build . --target UPDATE_SHADERS
    cmake --build . --target UPDATE_ASSETS
```
---

## PBR Demo instructions

* ```Q``` to quit the application
* ```[ ]``` to change camera type
* ```WSAD, MMB``` to control camera

![](screenshot.png)