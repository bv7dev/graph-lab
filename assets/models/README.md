# glTF Models Directory

Place glTF model files (.gltf or .glb) in this directory.

## Quick Start

### Download a Sample Model

You can download free glTF models from:

1. **Khronos glTF Sample Models** (Official samples):
   - https://github.com/KhronosGroup/glTF-Sample-Models/tree/master/2.0
   - Good starting models:
     - `Box/glTF/Box.gltf` - Simple colored cube
     - `Cube/glTF/Cube.gltf` - Basic cube
     - `Duck/glTF/Duck.gltf` - Classic test model

2. **Poly Haven** (Free high-quality models):
   - https://polyhaven.com/models

3. **Sketchfab** (Many free models):
   - https://sketchfab.com/3d-models?features=downloadable&sort_by=-likeCount

### Example: Download Box model

```bash
cd assets/models
wget https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/master/2.0/Box/glTF/Box.gltf
wget https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/master/2.0/Box/glTF/Box0.bin
```

Or download the binary version:
```bash
wget https://raw.githubusercontent.com/KhronosGroup/glTF-Sample-Models/master/2.0/Box/glTF-Binary/Box.glb
```

### Usage

Run the glTF viewer:
```bash
./build/gltf_viewer assets/models/Box.gltf
# or
./build/gltf_viewer assets/models/Box.glb
```

## File Formats

- **.gltf** - JSON text format (easier to inspect)
- **.glb** - Binary format (smaller, single file)

Both formats are supported by the viewer.
