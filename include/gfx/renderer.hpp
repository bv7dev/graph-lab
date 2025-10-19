#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <gfx/window.hpp>
#include <util/types.hpp>
#include <util/glm.hpp>
#include <gfx/mesh_renderer_opengl.hpp>

namespace gfx {

using namespace util;

// Template wrapper for compile-time polymorphism
template <typename ImplType>
class RendererTemplate {
 public:
  bool initialize(const Window& window, uint32_t width, uint32_t height) {
    return impl.initialize(window, width, height);
  }

  void setViewport(uint32_t width, uint32_t height) { impl.setViewport(width, height); }

  void clear(const Color& color = Color(0.0f, 0.0f, 0.0f, 1.0f)) { impl.clear(color); }

  void beginFrame() { impl.beginFrame(); }

  void endFrame() { impl.endFrame(); }

  float getFramerate() { return impl.getFramerate(); }

  void drawLine(float x1, float y1, float x2, float y2, const Color& color = Color()) {
    impl.drawLine(x1, y1, x2, y2, color);
  }

  void drawRectangle(float x, float y, float width, float height, const Color& color = Color(), bool filled = true) {
    impl.drawRectangle(x, y, width, height, color, filled);
  }

  void drawCircle(float centerX, float centerY, float radius, const Color& color = Color(), bool filled = true) {
    impl.drawCircle(centerX, centerY, radius, color, filled);
  }

  void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color& color = Color(),
                    bool filled = true) {
    impl.drawTriangle(x1, y1, x2, y2, x3, y3, color, filled);
  }

  void drawTriangles(const std::vector<Vertex2D>& vertices) { impl.drawTriangles(vertices); }

  void drawLines(const std::vector<Vertex2D>& vertices) { impl.drawLines(vertices); }

  // Texture management
  TextureGPU uploadTexture(const Texture& texture) { return impl.uploadTexture(texture); }

  void freeTexture(TextureGPU& textureGPU) { impl.freeTexture(textureGPU); }

  // Unified mesh rendering API - upload once, draw many times (works for 2D and 3D)
  MeshGPU uploadMesh(const Mesh3D& mesh) { return impl.uploadMesh(mesh); }

  void drawMesh(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f),
                bool wireframe = false) {
    impl.drawMesh(meshGPU, mvp, tint, wireframe);
  }

  void drawMeshPBR(const MeshGPU& meshGPU, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection,
                   const PBRMaterial& material, const glm::vec3& cameraPos, const glm::vec3& lightPos,
                   const Color& lightColor = Color(1.0f, 1.0f, 1.0f, 1.0f)) {
    impl.drawMeshPBR(meshGPU, model, view, projection, material, cameraPos, lightPos, lightColor);
  }

  void drawMeshEdges(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f),
                     float lineWidth = 1.0f) {
    impl.drawMeshEdges(meshGPU, mvp, tint, lineWidth);
  }

  void drawMeshPoints(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f),
                      float pointSize = 1.0f) {
    impl.drawMeshPoints(meshGPU, mvp, tint, pointSize);
  }

  void freeMesh(MeshGPU& meshGPU) { impl.freeMesh(meshGPU); }

  void setColor(const Color& color) { impl.setColor(color); }

  void setBlending(bool enabled) { impl.setBlending(enabled); }

 private:
  ImplType impl;
};

// Forward declaration
class MeshRendererOpenGL;

// Type aliases
using Renderer = RendererTemplate<MeshRendererOpenGL>;
using RendererPtr = std::unique_ptr<RendererTemplate<MeshRendererOpenGL>>;

}  // namespace gfx

// Include the implementation after the template definition
#include "mesh_renderer_opengl.hpp"
