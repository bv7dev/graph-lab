#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glad/glad.h>

#include <gfx/mesh_renderer_opengl.hpp>

#include <util/glm.hpp>
#include <util/types.hpp>

namespace gfx {

// Single unified shader - works for both 2D (orthographic) and 3D (perspective)
const std::string MESH_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;

uniform mat4 uMVP;

out vec4 vertexColor;

void main() {
    gl_Position = uMVP * vec4(aPosition, 1.0);
    vertexColor = aColor;
}
)";

const std::string MESH_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;

uniform vec4 uTint;

void main() {
    FragColor = vertexColor * uTint;
}
)";

// Point shader for rendering smooth circular points
const std::string POINT_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;

uniform mat4 uMVP;

out vec4 vertexColor;

void main() {
    gl_Position = uMVP * vec4(aPosition, 1.0);
    vertexColor = aColor;
}
)";

const std::string POINT_FRAGMENT_SHADER = R"(
#version 330 core
in vec4 vertexColor;
out vec4 FragColor;

uniform vec4 uTint;

void main() {
    // Calculate distance from center of point sprite
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);
    
    // Discard fragments outside the circle
    if (dist > 0.5) {
        discard;
    }
    
    // Smooth antialiasing at the edge
    float alpha = 1.0 - smoothstep(0.4, 0.5, dist);
    
    FragColor = vertexColor * uTint;
    FragColor.a *= alpha;
}
)";

MeshRendererOpenGL::MeshRendererOpenGL() : m_meshShaderProgram(0), m_pointShaderProgram(0) {}

MeshRendererOpenGL::~MeshRendererOpenGL() { cleanup(); }

void MeshRendererOpenGL::cleanup() {
  if (m_meshShaderProgram) {
    glDeleteProgram(m_meshShaderProgram);
    m_meshShaderProgram = 0;
  }
  if (m_pointShaderProgram) {
    glDeleteProgram(m_pointShaderProgram);
    m_pointShaderProgram = 0;
  }
}

bool MeshRendererOpenGL::loadMeshShaders() {
  return createShaderProgram(MESH_VERTEX_SHADER, MESH_FRAGMENT_SHADER, m_meshShaderProgram);
}

bool MeshRendererOpenGL::loadPointShaders() {
  return createShaderProgram(POINT_VERTEX_SHADER, POINT_FRAGMENT_SHADER, m_pointShaderProgram);
}

MeshGPU MeshRendererOpenGL::uploadMesh(const Mesh3D& mesh) {
  MeshGPU meshGPU;

  if (m_meshShaderProgram == 0 && !loadMeshShaders()) {
    return meshGPU;
  }

  if (mesh.vertices.empty()) {
    return meshGPU;
  }

  // Build vertex data from faces
  if (!mesh.faces.empty()) {
    std::vector<float> vertexData;
    for (size_t i = 0; i + 2 < mesh.faces.size(); i += 3) {
      uint32_t idx1 = mesh.faces[i];
      uint32_t idx2 = mesh.faces[i + 1];
      uint32_t idx3 = mesh.faces[i + 2];

      if (idx1 < mesh.vertices.size() && idx2 < mesh.vertices.size() && idx3 < mesh.vertices.size()) {
        for (auto idx : {idx1, idx2, idx3}) {
          const auto& v = mesh.vertices[idx];
          vertexData.insert(vertexData.end(),
                            {v.position.x, v.position.y, v.position.z, v.color.r, v.color.g, v.color.b, v.color.a});
        }
      }
    }

    if (!vertexData.empty()) {
      // Upload face data to GPU
      glGenVertexArrays(1, &meshGPU.vao);
      glGenBuffers(1, &meshGPU.vbo);
      glBindVertexArray(meshGPU.vao);
      glBindBuffer(GL_ARRAY_BUFFER, meshGPU.vbo);
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertexData.size() * sizeof(float)), vertexData.data(),
                   GL_STATIC_DRAW);

      // Position attribute (vec3)
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr);
      glEnableVertexAttribArray(0);

      // Color attribute (vec4)
      glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
      glEnableVertexAttribArray(1);

      glBindVertexArray(0);

      meshGPU.vertexCount = static_cast<uint32_t>(vertexData.size() / 7);
    }
  }

  // Build vertex data from edges
  if (!mesh.edges.empty()) {
    std::vector<float> edgeData;
    for (size_t i = 0; i + 1 < mesh.edges.size(); i += 2) {
      uint32_t idx1 = mesh.edges[i];
      uint32_t idx2 = mesh.edges[i + 1];

      if (idx1 < mesh.vertices.size() && idx2 < mesh.vertices.size()) {
        for (auto idx : {idx1, idx2}) {
          const auto& v = mesh.vertices[idx];
          edgeData.insert(edgeData.end(),
                          {v.position.x, v.position.y, v.position.z, v.color.r, v.color.g, v.color.b, v.color.a});
        }
      }
    }

    if (!edgeData.empty()) {
      // Upload edge data to GPU
      glGenVertexArrays(1, &meshGPU.edgeVao);
      glGenBuffers(1, &meshGPU.edgeVbo);
      glBindVertexArray(meshGPU.edgeVao);
      glBindBuffer(GL_ARRAY_BUFFER, meshGPU.edgeVbo);
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(edgeData.size() * sizeof(float)), edgeData.data(),
                   GL_STATIC_DRAW);

      // Position attribute (vec3)
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr);
      glEnableVertexAttribArray(0);

      // Color attribute (vec4)
      glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
      glEnableVertexAttribArray(1);

      glBindVertexArray(0);

      meshGPU.edgeVertexCount = static_cast<uint32_t>(edgeData.size() / 7);
    }
  }

  // Build vertex data for points (all vertices)
  if (!mesh.vertices.empty()) {
    std::vector<float> pointData;
    for (const auto& v : mesh.vertices) {
      pointData.insert(pointData.end(),
                       {v.position.x, v.position.y, v.position.z, v.color.r, v.color.g, v.color.b, v.color.a});
    }

    if (!pointData.empty()) {
      // Upload point data to GPU
      glGenVertexArrays(1, &meshGPU.pointVao);
      glGenBuffers(1, &meshGPU.pointVbo);
      glBindVertexArray(meshGPU.pointVao);
      glBindBuffer(GL_ARRAY_BUFFER, meshGPU.pointVbo);
      glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(pointData.size() * sizeof(float)), pointData.data(),
                   GL_STATIC_DRAW);

      // Position attribute (vec3)
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr);
      glEnableVertexAttribArray(0);

      // Color attribute (vec4)
      glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
      glEnableVertexAttribArray(1);

      glBindVertexArray(0);

      meshGPU.pointVertexCount = static_cast<uint32_t>(pointData.size() / 7);
    }
  }

  return meshGPU;
}

void MeshRendererOpenGL::drawMesh(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint, bool wireframe) {
  if (!meshGPU.isValid() || (m_meshShaderProgram == 0 && !const_cast<MeshRendererOpenGL*>(this)->loadMeshShaders())) {
    return;
  }

  useShader(m_meshShaderProgram);

  // Set MVP matrix
  int mvpLoc = glGetUniformLocation(m_meshShaderProgram, "uMVP");
  if (mvpLoc != -1) {
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
  }

  // Set tint color
  setUniformColor(m_meshShaderProgram, tint);

  // Enable depth testing and backface culling
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  // Wireframe mode if requested
  if (wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

  // Draw the mesh
  glBindVertexArray(meshGPU.vao);
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(meshGPU.vertexCount));
  glBindVertexArray(0);

  // Reset state
  if (wireframe) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
}

void MeshRendererOpenGL::drawMeshEdges(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint,
                                       float lineWidth) {
  if (!meshGPU.hasEdges() || (m_meshShaderProgram == 0 && !const_cast<MeshRendererOpenGL*>(this)->loadMeshShaders())) {
    return;
  }

  useShader(m_meshShaderProgram);

  // Set MVP matrix
  int mvpLoc = glGetUniformLocation(m_meshShaderProgram, "uMVP");
  if (mvpLoc != -1) {
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
  }

  // Set tint color
  setUniformColor(m_meshShaderProgram, tint);

  // Enable depth testing for proper 3D rendering
  glEnable(GL_DEPTH_TEST);

  // Set line width
  glLineWidth(lineWidth);

  // Draw the edges
  glBindVertexArray(meshGPU.edgeVao);
  glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(meshGPU.edgeVertexCount));
  glBindVertexArray(0);

  // Reset state
  glLineWidth(1.0f);
  glDisable(GL_DEPTH_TEST);
}

void MeshRendererOpenGL::drawMeshPoints(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint,
                                        float pointSize) {
  if (!meshGPU.hasPoints() ||
      (m_pointShaderProgram == 0 && !const_cast<MeshRendererOpenGL*>(this)->loadPointShaders())) {
    return;
  }

  useShader(m_pointShaderProgram);

  // Set MVP matrix
  int mvpLoc = glGetUniformLocation(m_pointShaderProgram, "uMVP");
  if (mvpLoc != -1) {
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
  }

  // Set tint color
  setUniformColor(m_pointShaderProgram, tint);

  // Enable depth testing for proper 3D rendering
  glEnable(GL_DEPTH_TEST);

  // Enable blending for smooth antialiased circles
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Enable point sprite mode for gl_PointCoord
  glEnable(GL_PROGRAM_POINT_SIZE);

  // Set point size
  glPointSize(pointSize);

  // Draw the points
  glBindVertexArray(meshGPU.pointVao);
  glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(meshGPU.pointVertexCount));
  glBindVertexArray(0);

  // Reset state
  glPointSize(1.0f);
  glDisable(GL_PROGRAM_POINT_SIZE);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
}

void MeshRendererOpenGL::freeMesh(MeshGPU& meshGPU) {
  if (meshGPU.vao) {
    glDeleteVertexArrays(1, &meshGPU.vao);
    glDeleteBuffers(1, &meshGPU.vbo);
    meshGPU.vao = 0;
    meshGPU.vbo = 0;
    meshGPU.vertexCount = 0;
  }
  if (meshGPU.edgeVao) {
    glDeleteVertexArrays(1, &meshGPU.edgeVao);
    glDeleteBuffers(1, &meshGPU.edgeVbo);
    meshGPU.edgeVao = 0;
    meshGPU.edgeVbo = 0;
    meshGPU.edgeVertexCount = 0;
  }
  if (meshGPU.pointVao) {
    glDeleteVertexArrays(1, &meshGPU.pointVao);
    glDeleteBuffers(1, &meshGPU.pointVbo);
    meshGPU.pointVao = 0;
    meshGPU.pointVbo = 0;
    meshGPU.pointVertexCount = 0;
  }
}

}  // namespace gfx
