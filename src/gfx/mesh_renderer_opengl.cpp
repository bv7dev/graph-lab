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

MeshRendererOpenGL::MeshRendererOpenGL() : m_meshShaderProgram(0) {}

MeshRendererOpenGL::~MeshRendererOpenGL() { cleanup(); }

void MeshRendererOpenGL::cleanup() {
  if (m_meshShaderProgram) {
    glDeleteProgram(m_meshShaderProgram);
    m_meshShaderProgram = 0;
  }
}

bool MeshRendererOpenGL::loadMeshShaders() {
  return createShaderProgram(MESH_VERTEX_SHADER, MESH_FRAGMENT_SHADER, m_meshShaderProgram);
}

MeshGPU MeshRendererOpenGL::uploadMesh(const Mesh3D& mesh) {
  MeshGPU meshGPU;

  if (m_meshShaderProgram == 0 && !loadMeshShaders()) {
    return meshGPU;
  }

  if (mesh.faces.empty() || mesh.vertices.empty()) {
    return meshGPU;
  }

  // Build vertex data from faces
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

  if (vertexData.empty()) {
    return meshGPU;
  }

  // Upload to GPU
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

void MeshRendererOpenGL::freeMesh(MeshGPU& meshGPU) {
  if (meshGPU.vao) {
    glDeleteVertexArrays(1, &meshGPU.vao);
    glDeleteBuffers(1, &meshGPU.vbo);
    meshGPU.vao = 0;
    meshGPU.vbo = 0;
    meshGPU.vertexCount = 0;
  }
}

}  // namespace gfx
