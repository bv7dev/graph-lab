#pragma once

#include <cstdint>
#include <vector>

#include <util/glm.hpp>

namespace util {

// Use GLM's vec4 for colors (supports .r, .g, .b, .a accessors)
using Color = glm::vec4;

struct Vertex2D {
  glm::vec2 position{0.0f, 0.0f};
  Color color{0.0f, 0.0f, 0.0f, 1.0f};

  Vertex2D() = default;

  Vertex2D(float x, float y, const Color& col = Color(0.0f, 0.0f, 0.0f, 1.0f)) : position(x, y), color(col) {}

  Vertex2D(const glm::vec2& pos, const Color& col = Color(0.0f, 0.0f, 0.0f, 1.0f)) : position(pos), color(col) {}
};

struct Vertex3D {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  Color color{0.0f, 0.0f, 0.0f, 1.0f};

  Vertex3D() = default;

  Vertex3D(float x, float y, float z, const Color& col = Color(0.0f, 0.0f, 0.0f, 1.0f))
      : position(x, y, z), color(col) {}

  Vertex3D(const glm::vec3& pos, const Color& col = Color(0.0f, 0.0f, 0.0f, 1.0f)) : position(pos), color(col) {}
};

// Simple mesh structure for 2D rendering
struct Mesh2D {
  std::vector<Vertex2D> vertices;
  std::vector<uint32_t> faces;  // Triplets of vertex indices (triangle = 3 indices)
  std::vector<uint32_t> edges;  // Pairs of vertex indices (line = 2 indices)

  Mesh2D() = default;

  // Helper to add a triangular face
  void addFace(uint32_t v1, uint32_t v2, uint32_t v3) {
    faces.push_back(v1);
    faces.push_back(v2);
    faces.push_back(v3);
  }

  // Helper to add a line edge
  void addEdge(uint32_t v1, uint32_t v2) {
    edges.push_back(v1);
    edges.push_back(v2);
  }
};

// Simple mesh structure for 3D rendering
struct Mesh3D {
  std::vector<Vertex3D> vertices;
  std::vector<uint32_t> faces;  // Triplets of vertex indices (triangle = 3 indices)
  std::vector<uint32_t> edges;  // Pairs of vertex indices (line = 2 indices)

  Mesh3D() = default;

  // Helper to add a triangular face
  void addFace(uint32_t v1, uint32_t v2, uint32_t v3) {
    faces.push_back(v1);
    faces.push_back(v2);
    faces.push_back(v3);
  }

  // Helper to add a line edge
  void addEdge(uint32_t v1, uint32_t v2) {
    edges.push_back(v1);
    edges.push_back(v2);
  }
};

// GPU mesh handle - holds VAO/VBO for a mesh that's been uploaded to GPU
// This allows us to upload once and draw many times efficiently
struct MeshGPU {
  uint32_t vao = 0;
  uint32_t vbo = 0;
  uint32_t vertexCount = 0;

  // Separate VAO/VBO for edges (lines)
  uint32_t edgeVao = 0;
  uint32_t edgeVbo = 0;
  uint32_t edgeVertexCount = 0;

  // Separate VAO/VBO for points (vertices)
  uint32_t pointVao = 0;
  uint32_t pointVbo = 0;
  uint32_t pointVertexCount = 0;

  [[nodiscard]] bool isValid() const { return vao != 0; }
  [[nodiscard]] bool hasEdges() const { return edgeVao != 0 && edgeVertexCount > 0; }
  [[nodiscard]] bool hasPoints() const { return pointVao != 0 && pointVertexCount > 0; }
};

}  // namespace util
