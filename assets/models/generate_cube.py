#!/usr/bin/env python3
"""Generate a simple colored cube glTF binary data"""

import struct

# Cube vertices (8 corners)
# Arranged as: back face (z=-1), front face (z=+1)
positions = [
    -1.0, -1.0, -1.0,  # 0: back-bottom-left
     1.0, -1.0, -1.0,  # 1: back-bottom-right
     1.0,  1.0, -1.0,  # 2: back-top-right
    -1.0,  1.0, -1.0,  # 3: back-top-left
    -1.0, -1.0,  1.0,  # 4: front-bottom-left
     1.0, -1.0,  1.0,  # 5: front-bottom-right
     1.0,  1.0,  1.0,  # 6: front-top-right
    -1.0,  1.0,  1.0,  # 7: front-top-left
]

# Vertex colors (RGBA)
colors = [
    0.0, 0.0, 0.0, 1.0,  # 0 - black
    1.0, 0.0, 0.0, 1.0,  # 1 - red
    1.0, 1.0, 0.0, 1.0,  # 2 - yellow
    0.0, 1.0, 0.0, 1.0,  # 3 - green
    0.0, 0.0, 1.0, 1.0,  # 4 - blue
    1.0, 0.0, 1.0, 1.0,  # 5 - magenta
    1.0, 1.0, 1.0, 1.0,  # 6 - white
    0.0, 1.0, 1.0, 1.0,  # 7 - cyan
]

# Indices for 12 triangles (6 faces * 2 triangles each)
# Counter-clockwise winding when viewed from OUTSIDE the cube
indices = [
    # Front face (z = +1, looking from +Z)
    4, 5, 6,  4, 6, 7,
    # Back face (z = -1, looking from -Z) 
    1, 0, 3,  1, 3, 2,
    # Right face (x = +1, looking from +X)
    5, 1, 2,  5, 2, 6,
    # Left face (x = -1, looking from -X)
    0, 4, 7,  0, 7, 3,
    # Top face (y = +1, looking from +Y)
    7, 6, 2,  7, 2, 3,
    # Bottom face (y = -1, looking from -Y)
    0, 1, 5,  0, 5, 4,
]

# Write binary file
with open('assets/models/cube.bin', 'wb') as f:
    # Write positions (96 bytes = 8 vertices * 3 floats * 4 bytes)
    for pos in positions:
        f.write(struct.pack('<f', pos))
    
    # Write colors (128 bytes = 8 vertices * 4 floats * 4 bytes)
    for color in colors:
        f.write(struct.pack('<f', color))
    
    # Write indices (72 bytes = 36 indices * 2 bytes as unsigned short)
    for idx in indices:
        f.write(struct.pack('<H', idx))

print("Generated cube.bin (296 bytes)")
