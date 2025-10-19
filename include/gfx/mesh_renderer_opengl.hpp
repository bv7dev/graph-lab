#pragma once

#include <cstdint>

#include <gfx/renderer_opengl.hpp>
#include <util/types.hpp>
#include <util/glm.hpp>

namespace gfx {

using namespace util;

class MeshRendererOpenGL : public RendererOpenGL {
 public:
  MeshRendererOpenGL();
  ~MeshRendererOpenGL();

  MeshRendererOpenGL(const MeshRendererOpenGL&) = delete;
  MeshRendererOpenGL(MeshRendererOpenGL&&) = delete;
  MeshRendererOpenGL& operator=(const MeshRendererOpenGL&) = delete;
  MeshRendererOpenGL& operator=(MeshRendererOpenGL&&) = delete;

  // Unified mesh rendering API - upload once, draw many times
  MeshGPU uploadMesh(const Mesh3D& mesh);

  // Draw uploaded mesh with MVP matrix (wireframe uses glPolygonMode)
  void drawMesh(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f),
                bool wireframe = false);

  // Draw edges from the mesh as lines
  void drawMeshEdges(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f),
                     float lineWidth = 1.0f);

  static void freeMesh(MeshGPU& meshGPU);

 private:
  uint32_t m_meshShaderProgram;

  void cleanup();
  bool loadMeshShaders();
};

}  // namespace gfx
