Project by Hlebov Oleksandr,
DAE student, group 2GD11E

[github repo](https://github.com/Howest-DAE-GD/graphics-programming-2-vulkan-project-OleksandrHlebov/tree/main)

Camera controls for dynamic rendering app:
  Holding Right Mouse Button:
    WASD -> movement
    E, Q -> move up, down
    SHIFT-> 2x movement speed
    Mouse movement controls the camera

Shaders get compiled automatically post-build, no user
input required.

Implemented features:  
1) Deferred rendering pipeline with a depth prepass
2) Cook-Torrance BRDF materials, loaded from gltf scene using assimp.
3) 2 light types:  
    * Point light
    * Directional light
4) Image based lighting for diffuse irradiance
5) Exposure based on physical camera setting
6) Tone mapping using Uncharted 2 operator for hdr mapping at the end of render
7) Shadow mapping for directional lights
