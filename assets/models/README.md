# glTF Models Directory

Place glTF model files (.gltf or .glb) in this directory.

## Quick Start

### Download Sample Models

You can download free glTF models from:

1. **Khronos glTF Sample Models** (Official samples):
   - https://github.com/KhronosGroup/glTF-Sample-Models/tree/master/2.0
   - Good starting models:
     - `Box/glTF/Box.gltf` - Simple colored cube
     - `Cube/glTF/Cube.gltf` - Basic cube
     - `Duck/glTF-Binary/Duck.glb` - Classic test model with texture
     - `DamagedHelmet/glTF-Binary/DamagedHelmet.glb` - Full PBR model with all textures

2. **Poly Haven** (Free high-quality models):
   - https://polyhaven.com/models

3. **Sketchfab** (Many free models):
   - https://sketchfab.com/3d-models?features=downloadable&sort_by=-likeCount

### Example: Download Models

```bash
cd assets/models

# Duck with texture (included in repo)
wget https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/master/2.0/Duck/glTF-Binary/Duck.glb

# Damaged Helmet - Full PBR model (included in repo)
wget https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/master/2.0/DamagedHelmet/glTF-Binary/DamagedHelmet.glb

# Simple Box
wget https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/master/2.0/Box/glTF-Binary/Box.glb
```

### Usage

Run the glTF viewer with PBR rendering:
```bash
# View textured models with PBR lighting
./build/gltf_viewer assets/models/Duck.glb
./build/gltf_viewer assets/models/DamagedHelmet.glb

# View simple colored cube
./build/gltf_viewer assets/models/cube.gltf
```

### Controls

The viewer includes:
- **Camera Controls**: Distance, angle, and height sliders
- **Model Controls**: Auto-rotation or manual rotation
- **Rendering Options**:
  - PBR Lighting (toggle on/off)
  - Wireframe mode
  - Show edges with adjustable line width
  - Show vertices as smooth points with adjustable size
- **Lighting Controls** (when PBR enabled):
  - Light distance, angle, and height
  - Light color

## File Formats

- **.gltf** - JSON text format (easier to inspect)
- **.glb** - Binary format (smaller, single file)

Both formats are supported by the viewer.
