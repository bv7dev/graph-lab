# PBR (Physically Based Rendering) Implementation

This document describes the full PBR implementation in the graph-lab renderer.

## Overview

The renderer now supports full PBR (Physically Based Rendering) with the metallic-roughness workflow, including:

- **Complete glTF 2.0 support** with textures
- **PBR metallic-roughness workflow**
- **Texture mapping** (base color, metallic-roughness, normal maps)
- **Cook-Torrance BRDF** lighting model
- **HDR tonemapping** and gamma correction
- **Interactive lighting controls**

## Features

### Supported Textures

1. **Base Color Texture** - Albedo/diffuse color map
2. **Metallic-Roughness Texture** - Combined metallic (blue channel) and roughness (green channel)
3. **Normal Map** - Surface detail via normal perturbation

### PBR Material Properties

Each material supports:
- Base color factor (vec4)
- Metallic factor (float, 0-1)
- Roughness factor (float, 0-1)
- Texture indices for all PBR maps

### Lighting Model

The implementation uses physically based Cook-Torrance BRDF with:

1. **Normal Distribution Function (NDF)**: GGX/Trowbridge-Reitz
   - Controls the distribution of microfacet normals
   - Determines specular highlight shape

2. **Geometry Function**: Schlick-GGX with Smith's method
   - Models self-shadowing and masking of microfacets
   - Separate calculation for view and light directions

3. **Fresnel Equation**: Fresnel-Schlick approximation
   - Determines reflection vs refraction ratio
   - Accounts for view angle dependency

4. **Energy Conservation**
   - Specular (kS) + Diffuse (kD) = 1.0
   - Metallic surfaces have no diffuse component

### Tonemapping and Color Correction

- **Reinhard tonemapping**: Maps HDR to LDR range
- **Gamma correction**: sRGB output with gamma 2.2
- **Linear workflow**: All calculations in linear space

## Architecture

### Data Structures

#### Vertex3D (types.hpp)
```cpp
struct Vertex3D {
  glm::vec3 position;  // XYZ coordinates
  Color color;         // RGBA color
  glm::vec3 normal;    // Surface normal
  glm::vec2 texCoord;  // UV coordinates
};
```

#### Texture (types.hpp)
```cpp
struct Texture {
  int width, height, channels;
  std::vector<unsigned char> data;
};
```

#### PBRMaterial (types.hpp)
```cpp
struct PBRMaterial {
  Color baseColor;
  float metallic;
  float roughness;
  uint32_t baseColorTexture;
  uint32_t metallicRoughnessTexture;
  uint32_t normalTexture;
};
```

### Shader Pipeline

#### Vertex Shader
- Transforms vertices to world and clip space
- Passes through normals (transformed by normal matrix)
- Passes through texture coordinates
- Calculates fragment position for lighting

#### Fragment Shader
- Samples all PBR textures
- Calculates view and light directions
- Computes Cook-Torrance BRDF
- Applies PBR lighting equation:
  ```
  Lo = (kD * baseColor/π + specular) * radiance * NdotL
  ```
- Adds ambient lighting
- Applies tonemapping and gamma correction

### API Usage

#### Loading Models

```cpp
#include <util/gltf_loader.hpp>

auto modelOpt = util::loadGLTF("path/to/model.glb");
if (modelOpt.has_value()) {
    util::Model& model = modelOpt.value();
    // model.meshes - geometry data
    // model.materials - material properties
    // model.textures - texture data
}
```

#### Uploading to GPU

```cpp
// Upload meshes
std::vector<util::MeshGPU> gpuMeshes;
for (const auto& mesh : model.meshes) {
    gpuMeshes.push_back(renderer.uploadMesh(mesh));
}

// Upload textures
std::vector<util::TextureGPU> gpuTextures;
for (const auto& texture : model.textures) {
    gpuTextures.push_back(renderer.uploadTexture(texture));
}
```

#### Rendering with PBR

```cpp
// Setup material
util::PBRMaterial material;
material.baseColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
material.metallic = 0.5f;
material.roughness = 0.5f;
material.baseColorTexture = gpuTextures[0].id;

// Setup matrices
glm::mat4 model = glm::mat4(1.0f);
glm::mat4 view = glm::lookAt(cameraPos, target, up);
glm::mat4 projection = glm::perspective(fov, aspect, near, far);

// Setup lighting
glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
glm::vec3 lightPos(10.0f, 10.0f, 10.0f);
Color lightColor(1.0f, 1.0f, 1.0f, 1.0f);

// Draw with PBR
renderer.drawMeshPBR(gpuMesh, model, view, projection, 
                     material, cameraPos, lightPos, lightColor);
```

#### Cleanup

```cpp
for (auto& gpuMesh : gpuMeshes) {
    renderer.freeMesh(gpuMesh);
}
for (auto& gpuTexture : gpuTextures) {
    renderer.freeTexture(gpuTexture);
}
```

## glTF Viewer Controls

The `gltf_viewer` application provides interactive controls:

### Camera
- **Distance**: Zoom in/out (1-20 units)
- **Angle**: Orbit around model (0-360°)
- **Height**: Vertical camera position (-5 to +5 units)

### Model
- **Auto Rotate**: Automatic turntable rotation
- **Rotation**: Manual rotation control when auto-rotate is off

### Rendering
- **PBR Lighting**: Toggle between PBR and simple rendering
- **Wireframe**: Show wireframe overlay
- **Show Edges**: Display mesh edges as colored lines
- **Show Points**: Display vertices as smooth circular points

### Lighting (PBR mode)
- **Light Distance**: How far the light is from origin (5-50 units)
- **Light Angle**: Orbital angle of light (0-360°)
- **Light Height**: Vertical position of light (-10 to +10 units)
- **Light Color**: RGB color of the light source

## Technical Details

### Vertex Layout

The vertex buffer layout is optimized for PBR rendering:
```
Position (vec3): 3 floats
Color (vec4):    4 floats
Normal (vec3):   3 floats
TexCoord (vec2): 2 floats
--------------------------
Total:          12 floats per vertex
```

### Texture Handling

- **Format detection**: Automatically handles RGB, RGBA, and grayscale
- **Mipmapping**: Automatic mipmap generation for better quality
- **Filtering**: Linear filtering with mipmaps for smooth appearance
- **Wrapping**: Repeat mode for tiled textures

### Performance Considerations

- **Single draw call per mesh**: Efficient GPU utilization
- **Indexed rendering**: Uses face indices to minimize vertex duplication
- **Backface culling**: Enabled in PBR mode for better performance
- **Depth testing**: Proper occlusion handling

## Tested Models

The implementation has been tested with:

1. **cube.gltf** - Simple colored cube (no textures)
   - Tests: Basic geometry, vertex colors, material factors

2. **Duck.glb** - Textured duck model
   - Tests: Base color texture, UV mapping, simple geometry

3. **DamagedHelmet.glb** - Full PBR model
   - Tests: All texture types, complex geometry, metallic-roughness workflow

## Future Enhancements

Potential improvements for future development:

### Advanced Lighting
- [ ] Multiple light sources
- [ ] Directional lights
- [ ] Point lights with falloff
- [ ] Spot lights with cone angle
- [ ] Environment lighting (IBL)
- [ ] Shadow mapping

### Material Features
- [ ] Emissive materials
- [ ] Ambient occlusion maps
- [ ] Clearcoat extension
- [ ] Sheen extension
- [ ] Transmission/glass materials

### Texture Features
- [ ] Anisotropic filtering
- [ ] Proper tangent space for normal maps
- [ ] Parallax occlusion mapping
- [ ] Texture compression (KTX2)

### Animation
- [ ] Skeletal animation
- [ ] Morph targets
- [ ] Animation playback controls

### Scene Features
- [ ] Multiple scenes
- [ ] Scene graph hierarchy
- [ ] Camera animations
- [ ] Instanced rendering

## References

- **glTF 2.0 Specification**: https://www.khronos.org/gltf/
- **PBR Theory**: https://learnopengl.com/PBR/Theory
- **Khronos glTF Samples**: https://github.com/KhronosGroup/glTF-Sample-Models
- **Real Shading in Unreal Engine 4**: Epic Games SIGGRAPH 2013

## Implementation Files

### Core Files
- `include/util/types.hpp` - Data structures (Vertex3D, Texture, PBRMaterial)
- `include/util/gltf_loader.hpp` - glTF loading interface
- `src/util/gltf_loader.cpp` - glTF parsing implementation
- `include/gfx/renderer.hpp` - Renderer API
- `include/gfx/mesh_renderer_opengl.hpp` - OpenGL renderer interface
- `src/gfx/mesh_renderer_opengl.cpp` - PBR shader and rendering implementation

### Application
- `src/apps/gltf_viewer.cpp` - Interactive PBR viewer with ImGui controls

### Documentation
- `assets/models/README.md` - Model download instructions
- `GLTF_IMPLEMENTATION.md` - Original glTF feature documentation
- `PBR_IMPLEMENTATION.md` - This document

## Build and Run

```bash
# Build the viewer
./scripts/dev.sh debug gltf_viewer

# Run with a model
./build/gltf_viewer assets/models/DamagedHelmet.glb

# Or with any glTF file
./build/gltf_viewer path/to/your/model.glb
```

## Conclusion

This PBR implementation provides a solid foundation for physically-based rendering in the graph-lab project. It supports the core features of glTF 2.0's metallic-roughness workflow and can render a wide variety of 3D models with realistic lighting and materials.
