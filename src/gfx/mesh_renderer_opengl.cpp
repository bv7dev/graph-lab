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

// PBR shader with textures and lighting
const std::string PBR_VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 FragPos;
out vec3 Normal;
out vec4 VertexColor;
out vec2 TexCoord;

void main() {
    FragPos = vec3(uModel * vec4(aPosition, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    VertexColor = aColor;
    TexCoord = aTexCoord;
    gl_Position = uProjection * uView * vec4(FragPos, 1.0);
}
)";

const std::string PBR_FRAGMENT_SHADER = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec4 VertexColor;
in vec2 TexCoord;

out vec4 FragColor;

// Material properties
uniform vec4 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;

// Textures
uniform sampler2D uBaseColorTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uNormalTexture;
uniform bool uHasBaseColorTexture;
uniform bool uHasMetallicRoughnessTexture;
uniform bool uHasNormalTexture;

// Lighting
uniform vec3 uCameraPos;
uniform vec3 uLightPos;
uniform vec4 uLightColor;

const float PI = 3.14159265359;

// Normal Distribution Function (GGX/Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

// Geometry Function (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    // Sample textures
    vec4 baseColor = uBaseColor * VertexColor;
    if (uHasBaseColorTexture) {
        baseColor *= texture(uBaseColorTexture, TexCoord);
    }
    
    float metallic = uMetallic;
    float roughness = uRoughness;
    if (uHasMetallicRoughnessTexture) {
        vec4 mr = texture(uMetallicRoughnessTexture, TexCoord);
        roughness *= mr.g;  // Green channel = roughness
        metallic *= mr.b;   // Blue channel = metallic
    }
    
    vec3 N = normalize(Normal);
    if (uHasNormalTexture) {
        // Sample normal map and transform to world space
        vec3 tangentNormal = texture(uNormalTexture, TexCoord).xyz * 2.0 - 1.0;
        // For simplicity, we're not doing proper tangent space here
        // In a full implementation, you'd need tangent and bitangent vectors
        N = normalize(N + tangentNormal * 0.1);
    }
    
    vec3 V = normalize(uCameraPos - FragPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor.rgb, metallic);
    
    // Lighting calculation
    vec3 Lo = vec3(0.0);
    
    // Single light for now
    vec3 L = normalize(uLightPos - FragPos);
    vec3 H = normalize(V + L);
    float distance = length(uLightPos - FragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = uLightColor.rgb * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * baseColor.rgb / PI + specular) * radiance * NdotL;
    
    // Ambient lighting
    vec3 ambient = vec3(0.03) * baseColor.rgb;
    vec3 color = ambient + Lo;
    
    // HDR tonemapping and gamma correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, baseColor.a);
}
)";

MeshRendererOpenGL::MeshRendererOpenGL() : m_meshShaderProgram(0), m_pbrShaderProgram(0), m_pointShaderProgram(0) {}

MeshRendererOpenGL::~MeshRendererOpenGL() { cleanup(); }

void MeshRendererOpenGL::cleanup() {
  if (m_meshShaderProgram) {
    glDeleteProgram(m_meshShaderProgram);
    m_meshShaderProgram = 0;
  }
  if (m_pbrShaderProgram) {
    glDeleteProgram(m_pbrShaderProgram);
    m_pbrShaderProgram = 0;
  }
  if (m_pointShaderProgram) {
    glDeleteProgram(m_pointShaderProgram);
    m_pointShaderProgram = 0;
  }
}

bool MeshRendererOpenGL::loadMeshShaders() {
  return createShaderProgram(MESH_VERTEX_SHADER, MESH_FRAGMENT_SHADER, m_meshShaderProgram);
}

bool MeshRendererOpenGL::loadPBRShaders() {
  return createShaderProgram(PBR_VERTEX_SHADER, PBR_FRAGMENT_SHADER, m_pbrShaderProgram);
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
  // Layout: position(3) + color(4) + normal(3) + texCoord(2) = 12 floats per vertex
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
                            {v.position.x, v.position.y, v.position.z, v.color.r, v.color.g, v.color.b, v.color.a,
                             v.normal.x, v.normal.y, v.normal.z, v.texCoord.x, v.texCoord.y});
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

      const size_t stride = 12 * sizeof(float);

      // Position attribute (vec3)
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
      glEnableVertexAttribArray(0);

      // Color attribute (vec4)
      glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
      glEnableVertexAttribArray(1);

      // Normal attribute (vec3)
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(7 * sizeof(float)));
      glEnableVertexAttribArray(2);

      // TexCoord attribute (vec2)
      glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(10 * sizeof(float)));
      glEnableVertexAttribArray(3);

      glBindVertexArray(0);

      meshGPU.vertexCount = static_cast<uint32_t>(vertexData.size() / 12);
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

TextureGPU MeshRendererOpenGL::uploadTexture(const Texture& texture) {
  TextureGPU textureGPU;

  if (!texture.isValid()) {
    return textureGPU;
  }

  glGenTextures(1, &textureGPU.id);
  glBindTexture(GL_TEXTURE_2D, textureGPU.id);

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Determine format
  GLenum format = GL_RGB;
  if (texture.channels == 1) {
    format = GL_RED;
  } else if (texture.channels == 3) {
    format = GL_RGB;
  } else if (texture.channels == 4) {
    format = GL_RGBA;
  }

  // Upload texture data
  glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), texture.width, texture.height, 0, format, GL_UNSIGNED_BYTE,
               texture.data.data());
  glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);

  textureGPU.width = texture.width;
  textureGPU.height = texture.height;
  textureGPU.channels = texture.channels;

  return textureGPU;
}

void MeshRendererOpenGL::freeTexture(TextureGPU& textureGPU) {
  if (textureGPU.id) {
    glDeleteTextures(1, &textureGPU.id);
    textureGPU.id = 0;
    textureGPU.width = 0;
    textureGPU.height = 0;
    textureGPU.channels = 0;
  }
}

void MeshRendererOpenGL::drawMeshPBR(const MeshGPU& meshGPU, const glm::mat4& model, const glm::mat4& view,
                                     const glm::mat4& projection, const PBRMaterial& material,
                                     const glm::vec3& cameraPos, const glm::vec3& lightPos, const Color& lightColor) {
  if (!meshGPU.isValid()) {
    return;
  }

  if (m_pbrShaderProgram == 0 && !loadPBRShaders()) {
    return;
  }

  glUseProgram(m_pbrShaderProgram);

  // Set matrices
  glUniformMatrix4fv(glGetUniformLocation(m_pbrShaderProgram, "uModel"), 1, GL_FALSE, &model[0][0]);
  glUniformMatrix4fv(glGetUniformLocation(m_pbrShaderProgram, "uView"), 1, GL_FALSE, &view[0][0]);
  glUniformMatrix4fv(glGetUniformLocation(m_pbrShaderProgram, "uProjection"), 1, GL_FALSE, &projection[0][0]);

  // Set material properties
  glUniform4f(glGetUniformLocation(m_pbrShaderProgram, "uBaseColor"), material.baseColor.r, material.baseColor.g,
              material.baseColor.b, material.baseColor.a);
  glUniform1f(glGetUniformLocation(m_pbrShaderProgram, "uMetallic"), material.metallic);
  glUniform1f(glGetUniformLocation(m_pbrShaderProgram, "uRoughness"), material.roughness);

  // Set lighting
  glUniform3f(glGetUniformLocation(m_pbrShaderProgram, "uCameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
  glUniform3f(glGetUniformLocation(m_pbrShaderProgram, "uLightPos"), lightPos.x, lightPos.y, lightPos.z);
  glUniform4f(glGetUniformLocation(m_pbrShaderProgram, "uLightColor"), lightColor.r, lightColor.g, lightColor.b,
              lightColor.a);

  // Bind textures
  bool hasBaseColorTexture = material.baseColorTexture != 0;
  bool hasMetallicRoughnessTexture = material.metallicRoughnessTexture != 0;
  bool hasNormalTexture = material.normalTexture != 0;

  glUniform1i(glGetUniformLocation(m_pbrShaderProgram, "uHasBaseColorTexture"), hasBaseColorTexture ? 1 : 0);
  glUniform1i(glGetUniformLocation(m_pbrShaderProgram, "uHasMetallicRoughnessTexture"),
              hasMetallicRoughnessTexture ? 1 : 0);
  glUniform1i(glGetUniformLocation(m_pbrShaderProgram, "uHasNormalTexture"), hasNormalTexture ? 1 : 0);

  if (hasBaseColorTexture) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material.baseColorTexture);
    glUniform1i(glGetUniformLocation(m_pbrShaderProgram, "uBaseColorTexture"), 0);
  }

  if (hasMetallicRoughnessTexture) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, material.metallicRoughnessTexture);
    glUniform1i(glGetUniformLocation(m_pbrShaderProgram, "uMetallicRoughnessTexture"), 1);
  }

  if (hasNormalTexture) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, material.normalTexture);
    glUniform1i(glGetUniformLocation(m_pbrShaderProgram, "uNormalTexture"), 2);
  }

  // Enable depth testing and backface culling
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  // Draw mesh
  glBindVertexArray(meshGPU.vao);
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(meshGPU.vertexCount));
  glBindVertexArray(0);

  // Reset state
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glActiveTexture(GL_TEXTURE0);
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
