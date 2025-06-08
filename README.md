# OpenGL Graphics Engine

This project is a C++ graphics engine built with OpenGL 4.3. It showcases advanced rendering techniques and interactive UI using ImGui.

## ✨ Main Features

### 🧱 Entity System
- Model loading via Assimp.
- Dynamic entity positioning and transformation (`glm::translate`, `glm::rotate`, etc.).
- Entity types:
  - `Deferred_Rendering`
  - `Relief_Mapping`
  - `Environment_Map`
- Animation support (e.g., rotating Pikachu and relief-mapped cubes).

### 🔦 Lighting System
- Supports:
  - Directional Lights
  - Point Lights
- Real-time light creation and deletion via UI.
- Up to 400 dynamic lights per rendering mode.
- Customizable color, direction, position, and intensity.

### 🎮 Camera Controls
- First-person movement (`W`, `A`, `S`, `D`, `Q`, `E`).
- Mouse-driven rotation.
- "Gravitational Camera" mode (key `F`) always looks at the origin.
- Adjustable speed and sensitivity via UI.

## 🖼️ Rendering Modes

### 1. Deferred Rendering
- G-Buffer setup:
  - Albedo
  - Normals
  - Position
  - View Direction
  - Depth
- Buffer selection viewable via UI.
- Deferred lighting with many dynamic lights.

### 2. Relief Mapping
- Realistic implementation using normal and height maps.
- Toggleable view modes:
  - `Main`
  - `Normal`
  - `Height`
- Adjustable relief intensity (`uHeightScale`) via slider.

### 3. Environment Mapping
- Reflection and refraction via cube maps.
- Three preloaded cube maps: `Outdoor`, `House`, `Studio`.
- Toggle between `Reflection` and `Refraction` modes.
- Adjustable diffuse ambient intensity (`diffus_amb`).

## 🧰 Additional Features
- Switch between `Forward Rendering` and `Deferred Rendering`.
- Skybox rendering with proper depth behavior.
- Live shader reloading when GLSL files are edited.
- Real-time entity and light inspector.
- Display available OpenGL extensions.
- Real-time FPS monitor.

## 🧠 Included Shaders

| Shader File                  | Description                       |
|------------------------------|-----------------------------------|
| `FORWARD.glsl`               | Forward rendering shader          |
| `RENDER_GEOMETRY.glsl`       | Geometry pass (deferred)          |
| `Render_Quad.glsl`           | Final composition for G-Buffers   |
| `Relief_Mapping.glsl`        | Relief mapping with view modes    |
| `Reflection_environment.glsl`| Environment mapping (reflections/refractions) |
| `CubeMap.glsl`               | Skybox rendering                  |

## 🧾 Requirements

- OpenGL 4.3+
- [GLM](https://github.com/g-truc/glm)
- [ImGui](https://github.com/ocornut/imgui)
- [stb_image](https://github.com/nothings/stb)
- [Assimp](https://github.com/assimp/assimp)

- G-Buffer outputs (Albedo, Normals, Depth...).
![435467616-3c80fd53-84d1-4c7c-bbd8-3fe556ee3b2e](https://github.com/user-attachments/assets/4f98cd6f-9c26-4bcc-b55a-5c07c1ceb3f9)
![435467717-5f13eb11-528b-468d-a2b9-8f8549f1cf10](https://github.com/user-attachments/assets/0ebb70b5-ace5-433a-b8ab-901f9b56b527)
- Relief mapped cubes with different scales and filters:
-Albedo
![Image_1](https://github.com/user-attachments/assets/a371865e-935a-49ef-9f76-2781f524f17f)
-Normal
![Image_2](https://github.com/user-attachments/assets/5c4f4b9e-740a-4e21-a007-f8f5628748aa)
-Height
![Image_3](https://github.com/user-attachments/assets/db8f20de-de06-4cf2-ae17-b3ab48106862)
- Full skybox and environment setup.
![Image_4](https://github.com/user-attachments/assets/2f3b6725-f38c-4cce-8287-9d98bbbf06cf)
- Reflections using different cube maps.
![Image_5](https://github.com/user-attachments/assets/f121c2bf-42d1-44b9-9d06-e62b2b66ec9a)
