#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <gfx/renderer_opengl.hpp>
#include <gfx/window.hpp>
#include <util/types.hpp>

namespace gfx {

// Shader sources
const std::string BASIC_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec4 aColor;

uniform mat4 uProjection;

out vec4 vertexColor;

void main() {
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
    vertexColor = aColor;
}
)";

const std::string BASIC_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vertexColor;
}
)";

const std::string LINE_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec4 aColor;

uniform mat4 uProjection;

out vec4 vertexColor;

void main() {
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
    vertexColor = aColor;
}
)";

const std::string LINE_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vertexColor;
}
)";

RendererOpenGL::RendererOpenGL()
    : m_width(0),
      m_height(0),
      m_initialized(false),
      m_currentColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_blendingEnabled(false),
      m_basicShaderProgram(0),
      m_lineShaderProgram(0),
      m_quadVAO(0),
      m_quadVBO(0),
      m_lineVAO(0),
      m_lineVBO(0),
      m_triangleVAO(0),
      m_triangleVBO(0),
      m_circleVAO(0),
      m_circleVBO(0) {}

RendererOpenGL::~RendererOpenGL() { cleanup(); }

bool RendererOpenGL::initialize(const Window& window, uint32_t width, uint32_t height) {
  if (m_initialized) {
    return false;
  }

  // Initialize GLAD
  if (!gladLoadGL()) {
    return false;
  }

  m_width = width;
  m_height = height;

  // Load shaders
  if (!loadShaders()) {
    return false;
  }

  // Setup geometry
  setupQuadGeometry();
  setupLineGeometry();
  setupTriangleGeometry();
  setupCircleGeometry();

  // Enable blending by default
  setBlending(true);

  // Initialize ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  // Get GLFW window handle for ImGui
  GLFWwindow* glfwWindow = window.getNativeWindow();

  ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  m_initialized = true;
  return true;
}

void RendererOpenGL::setViewport(uint32_t width, uint32_t height) {
  m_width = width;
  m_height = height;
  glViewport(0, 0, static_cast<int>(width), static_cast<int>(height));
}

void RendererOpenGL::clear(const Color& color) {
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

float RendererOpenGL::getFramerate() {
  ImGuiIO& io = ImGui::GetIO();
  return io.Framerate;
}

void RendererOpenGL::beginFrame() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void RendererOpenGL::endFrame() {
  // Render ImGui
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RendererOpenGL::drawLine(float x1, float y1, float x2, float y2, const Color& color) {
  useShader(m_lineShaderProgram);
  setUniformColor(m_lineShaderProgram, color);
  updateProjectionMatrix(m_lineShaderProgram);

  // Create line vertices
  float vertices[] = {x1, y1, color.r, color.g, color.b, color.a, x2, y2, color.r, color.g, color.b, color.a};

  glBindVertexArray(m_lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

  glDrawArrays(GL_LINES, 0, 2);
  glBindVertexArray(0);
}

void RendererOpenGL::drawRectangle(float x, float y, float width, float height, const Color& color, bool filled) {
  useShader(m_basicShaderProgram);
  setUniformColor(m_basicShaderProgram, color);
  updateProjectionMatrix(m_basicShaderProgram);

  // Create quad vertices (two triangles)
  float vertices[] = {
      x,         y,          color.r, color.g, color.b, color.a,  // bottom-left
      x + width, y,          color.r, color.g, color.b, color.a,  // bottom-right
      x,         y + height, color.r, color.g, color.b, color.a,  // top-left
      x + width, y + height, color.r, color.g, color.b, color.a,  // top-right
      x + width, y,          color.r, color.g, color.b, color.a,  // bottom-right
      x,         y + height, color.r, color.g, color.b, color.a   // top-left
  };

  glBindVertexArray(m_quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

  if (filled) {
    glDrawArrays(GL_TRIANGLES, 0, 6);
  } else {
    // Draw as line loop for outline
    float lineVertices[] = {
        x,         y,          color.r, color.g, color.b, color.a,  // bottom-left
        x + width, y,          color.r, color.g, color.b, color.a,  // bottom-right
        x + width, y + height, color.r, color.g, color.b, color.a,  // top-right
        x,         y + height, color.r, color.g, color.b, color.a   // top-left
    };
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
  }
  glBindVertexArray(0);
}

void RendererOpenGL::drawCircle(float centerX, float centerY, float radius, const Color& color, bool filled) {
  useShader(m_basicShaderProgram);
  setUniformColor(m_basicShaderProgram, color);
  updateProjectionMatrix(m_basicShaderProgram);

  const int segments = 32;
  std::vector<float> vertices;

  if (filled) {
    // Center vertex
    vertices.insert(vertices.end(), {centerX, centerY, color.r, color.g, color.b, color.a});

    // Circle vertices
    for (int i = 0; i <= segments; ++i) {
      float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
      float x = centerX + (radius * cosf(angle));
      float y = centerY + (radius * sinf(angle));
      vertices.insert(vertices.end(), {x, y, color.r, color.g, color.b, color.a});
    }
  } else {
    // Line loop for outline
    for (int i = 0; i <= segments; ++i) {
      float angle = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(segments);
      float x = centerX + (radius * cosf(angle));
      float y = centerY + (radius * sinf(angle));
      vertices.insert(vertices.end(), {x, y, color.r, color.g, color.b, color.a});
    }
  }

  glBindVertexArray(m_circleVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_circleVBO);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(),
               GL_DYNAMIC_DRAW);

  if (filled) {
    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);
  } else {
    glDrawArrays(GL_LINE_LOOP, 0, segments + 1);
  }
  glBindVertexArray(0);
}

void RendererOpenGL::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color& color,
                                  bool filled) {
  useShader(m_basicShaderProgram);
  setUniformColor(m_basicShaderProgram, color);
  updateProjectionMatrix(m_basicShaderProgram);

  float vertices[] = {x1,      y1,      color.r, color.g, color.b, color.a, x2,      y2,      color.r,
                      color.g, color.b, color.a, x3,      y3,      color.r, color.g, color.b, color.a};

  glBindVertexArray(m_triangleVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_triangleVBO);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

  if (filled) {
    glDrawArrays(GL_TRIANGLES, 0, 3);
  } else {
    glDrawArrays(GL_LINE_LOOP, 0, 3);
  }
  glBindVertexArray(0);
}

void RendererOpenGL::drawTriangles(const std::vector<Vertex2D>& vertices) {
  useShader(m_basicShaderProgram);
  setUniformColor(m_basicShaderProgram, m_currentColor);
  updateProjectionMatrix(m_basicShaderProgram);

  std::vector<float> vertexData;
  for (const auto& vertex : vertices) {
    vertexData.insert(vertexData.end(), {vertex.position.x, vertex.position.y, vertex.color.r, vertex.color.g,
                                         vertex.color.b, vertex.color.a});
  }

  glBindVertexArray(m_triangleVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_triangleVBO);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexData.size() * sizeof(float)), vertexData.data(),
               GL_DYNAMIC_DRAW);

  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
  glBindVertexArray(0);
}

void RendererOpenGL::drawLines(const std::vector<Vertex2D>& vertices) {
  useShader(m_lineShaderProgram);
  setUniformColor(m_lineShaderProgram, m_currentColor);
  updateProjectionMatrix(m_lineShaderProgram);

  std::vector<float> vertexData;
  for (const auto& vertex : vertices) {
    vertexData.insert(vertexData.end(), {vertex.position.x, vertex.position.y, vertex.color.r, vertex.color.g,
                                         vertex.color.b, vertex.color.a});
  }

  glBindVertexArray(m_lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexData.size() * sizeof(float)), vertexData.data(),
               GL_DYNAMIC_DRAW);

  glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
  glBindVertexArray(0);
}

void RendererOpenGL::setColor(const Color& color) { m_currentColor = color; }

void RendererOpenGL::setBlending(bool enabled) {
  m_blendingEnabled = enabled;
  if (enabled) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    glDisable(GL_BLEND);
  }
}

void RendererOpenGL::cleanup() {
  if (!m_initialized) {
    return;
  }

  // Cleanup VAOs and VBOs
  if (m_quadVAO) {
    glDeleteVertexArrays(1, &m_quadVAO);
  }
  if (m_quadVBO) {
    glDeleteBuffers(1, &m_quadVBO);
  }
  if (m_lineVAO) {
    glDeleteVertexArrays(1, &m_lineVAO);
  }
  if (m_lineVBO) {
    glDeleteBuffers(1, &m_lineVBO);
  }
  if (m_triangleVAO) {
    glDeleteVertexArrays(1, &m_triangleVAO);
  }
  if (m_triangleVBO) {
    glDeleteBuffers(1, &m_triangleVBO);
  }
  if (m_circleVAO) {
    glDeleteVertexArrays(1, &m_circleVAO);
  }
  if (m_circleVBO) {
    glDeleteBuffers(1, &m_circleVBO);
  }

  // Cleanup shaders
  if (m_basicShaderProgram) {
    glDeleteProgram(m_basicShaderProgram);
  }
  if (m_lineShaderProgram) {
    glDeleteProgram(m_lineShaderProgram);
  }

  // Cleanup ImGui
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  m_initialized = false;
}

bool RendererOpenGL::loadShaders() {
  // Load basic shader program
  if (!createShaderProgram(BASIC_VERTEX_SHADER, BASIC_FRAGMENT_SHADER, m_basicShaderProgram)) {
    return false;
  }

  // Load line shader program
  if (!createShaderProgram(LINE_VERTEX_SHADER, LINE_FRAGMENT_SHADER, m_lineShaderProgram)) {
    return false;
  }

  return true;
}

bool RendererOpenGL::createShaderProgram(const std::string& vertexSource, const std::string& fragmentSource,
                                         uint32_t& program) {
  uint32_t vertexShader = 0;
  uint32_t fragmentShader = 0;

  // Compile vertex shader
  if (!compileShader(vertexSource, GL_VERTEX_SHADER, vertexShader)) {
    return false;
  }

  // Compile fragment shader
  if (!compileShader(fragmentSource, GL_FRAGMENT_SHADER, fragmentShader)) {
    glDeleteShader(vertexShader);
    return false;
  }

  // Create shader program
  program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  // Check linking status
  int success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    glDeleteProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return false;
  }

  // Clean up individual shaders
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return true;
}

bool RendererOpenGL::compileShader(const std::string& source, uint32_t type, uint32_t& shader) {
  shader = glCreateShader(type);
  const char* src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  // Check compilation status
  int success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    glDeleteShader(shader);
    return false;
  }

  return true;
}

void RendererOpenGL::setupQuadGeometry() {
  glGenVertexArrays(1, &m_quadVAO);
  glGenBuffers(1, &m_quadVBO);

  glBindVertexArray(m_quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
  glBufferData(GL_ARRAY_BUFFER, 6 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // Color attribute
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void RendererOpenGL::setupLineGeometry() {
  glGenVertexArrays(1, &m_lineVAO);
  glGenBuffers(1, &m_lineVBO);

  glBindVertexArray(m_lineVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
  glBufferData(GL_ARRAY_BUFFER, 2 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // Color attribute
  constexpr size_t colorOffset = 2 * sizeof(float);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<const void*>(colorOffset));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void RendererOpenGL::setupTriangleGeometry() {
  glGenVertexArrays(1, &m_triangleVAO);
  glGenBuffers(1, &m_triangleVBO);

  glBindVertexArray(m_triangleVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_triangleVBO);
  glBufferData(GL_ARRAY_BUFFER, 3 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

  // Position attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // Color attribute
  constexpr size_t colorOffset = 2 * sizeof(float);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<const void*>(colorOffset));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void RendererOpenGL::setupCircleGeometry() {
  glGenVertexArrays(1, &m_circleVAO);
  glGenBuffers(1, &m_circleVBO);

  glBindVertexArray(m_circleVAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_circleVBO);

  // Position attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  // Color attribute
  constexpr size_t colorOffset = 2 * sizeof(float);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<const void*>(colorOffset));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

void RendererOpenGL::useShader(uint32_t program) { glUseProgram(program); }

void RendererOpenGL::setUniformColor(uint32_t program, const Color& color) {
  // Set the uniform tint color in the shader
  int location = glGetUniformLocation(program, "uTint");
  if (location != -1) {
    glUniform4f(location, color.r, color.g, color.b, color.a);
  }
}

void RendererOpenGL::updateProjectionMatrix(uint32_t program) const {
  // Create orthographic projection matrix using GLM
  // Bottom-left is (0,0), top-right is (width, height) - standard OpenGL convention
  glm::mat4 projection =
      glm::ortho(0.0f, static_cast<float>(getWidth()), 0.0f, static_cast<float>(getHeight()), -1.0f, 1.0f);

  int location = glGetUniformLocation(program, "uProjection");
  if (location != -1) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(projection));
  }
}

}  // namespace gfx
