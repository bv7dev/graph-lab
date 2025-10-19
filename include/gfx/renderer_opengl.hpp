#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <util/types.hpp>

#include <gfx/window.hpp>

namespace gfx {

using namespace util;

class RendererOpenGL {
 public:
  RendererOpenGL();
  ~RendererOpenGL();

  RendererOpenGL(const RendererOpenGL&) = delete;
  RendererOpenGL(RendererOpenGL&&) = delete;
  RendererOpenGL& operator=(const RendererOpenGL&) = delete;
  RendererOpenGL& operator=(RendererOpenGL&&) = delete;

  bool initialize(const Window& window, uint32_t width, uint32_t height);
  void setViewport(uint32_t width, uint32_t height);
  static void clear(const Color& color);
  static void beginFrame();
  static void endFrame();

  static float getFramerate();

  // Viewport accessors
  [[nodiscard]] uint32_t getWidth() const { return m_width; }
  [[nodiscard]] uint32_t getHeight() const { return m_height; }

  void drawLine(float x1, float y1, float x2, float y2, const Color& color);
  void drawRectangle(float x, float y, float width, float height, const Color& color, bool filled);
  void drawCircle(float centerX, float centerY, float radius, const Color& color, bool filled);
  void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color& color, bool filled);
  void drawTriangles(const std::vector<Vertex2D>& vertices);
  void drawLines(const std::vector<Vertex2D>& vertices);

  void setColor(const Color& color);
  void setBlending(bool enabled);

 private:
  // OpenGL state
  uint32_t m_width;
  uint32_t m_height;
  bool m_initialized;
  Color m_currentColor;
  bool m_blendingEnabled;

  // Shader program handles
  uint32_t m_basicShaderProgram;
  uint32_t m_lineShaderProgram;

  // VAO/VBO handles for different primitives
  uint32_t m_quadVAO;
  uint32_t m_quadVBO;
  uint32_t m_lineVAO;
  uint32_t m_lineVBO;
  uint32_t m_triangleVAO;
  uint32_t m_triangleVBO;
  uint32_t m_circleVAO;
  uint32_t m_circleVBO;

  void cleanup();

  bool loadShaders();
  static bool compileShader(const std::string& source, uint32_t type, uint32_t& shader);

  void setupQuadGeometry();
  void setupLineGeometry();
  void setupTriangleGeometry();
  void setupCircleGeometry();

 protected:
  static void useShader(uint32_t program);
  static bool createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource,
                                  uint32_t& program);
  static void setUniformColor(uint32_t program, const Color& color);
  void updateProjectionMatrix(uint32_t program) const;
};

}  // namespace gfx
