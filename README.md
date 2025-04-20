# AGP_Engine
# OpenGL Scene Renderer

This project is an OpenGL-based rendering engine designed for real-time graphics experiments.  
The main scene is composed of five 3D models, a plane, and a skybox — all loaded during the `Init()` function.

---

## 🎯 Features

- **Light Creation & Editing**  
  Create and delete lights dynamically using ImGui.  
  You can edit:
  - Light position
  - Light color
  - Light intensity  
  For `PointLights`, you can also adjust their direction.

- **Shader Hot Reloading**  
  Edit GLSL shaders and reload them live at runtime without restarting the program.

- **Light Stress Test**  
  Calculates the maximum number of lights your PC can handle based on your GPU's uniform buffer size.

- **Framebuffer Object with Multiple Render Targets**  
  The engine uses an FBO setup with the following textures:
  - Albedo
  - Normals
  - World Position
  - View Direction
  - Depth

- **Interactive 3D Camera**  
  Supports full navigation:
  - `WASD` keys: Move around the scene.
  - Left-click + Drag: Rotate the camera.
  - Right-click + Drag: Pan the camera in 2D.
  - You can adjust position, speed, and mouse sensitivity through ImGui.

---

## ⚙️ Build Instructions

1. Requires **OpenGL 4.3** or higher.
2. Dependencies:
   - [Assimp](https://www.assimp.org/) — Model loading.
   - [GLM](https://github.com/g-truc/glm) — Mathematics.
   - [Dear ImGui](https://github.com/ocornut/imgui) — User Interface.
   - [STB Image](https://github.com/nothings/stb) — Image loading.
3. You have to be sure to got your GPU on integrated to run the project well.

> 💡 **Note:**  
> To change the project to dedicated GPU to integrated:
> System -> Display -> Graphics -> Add the engine.exe and chang the GPU mode to the integrated


5. Clone the repository and build with your preferred C++ IDE or with CMake.

---

## 🧠 Notes

- Models and assets must be placed in the correct directory structure for the loader to work.
- The `UpdateLights()` function will limit the number of lights based on your GPU capacity, logging a warning if the limit is exceeded.
- The camera system and the entity setup are fully extensible for future improvements.

---

## 📸 Screenshots

![{80262BB2-1A06-46A7-BBAE-0B8035ECFA99}](https://github.com/user-attachments/assets/3c80fd53-84d1-4c7c-bbd8-3fe556ee3b2e)
![{C24FE3A9-B1C0-410A-AB9B-5A764EA3D644}](https://github.com/user-attachments/assets/5f13eb11-528b-468d-a2b9-8f8549f1cf10)
![{036A48D3-FB19-46C7-8637-FA6B4F71B86F}](https://github.com/user-attachments/assets/c398d404-1b93-4dc3-9e6b-560e6a7e1696)

---

## 💡 Author's Tip

The ImGui interface allows you to dynamically:
- Add and delete lights.
- Change camera properties.
- Trigger shader hot reloads.

It’s designed for easy testing and quick iteration!

---

