#pragma once

#include <string>
#include <vector>
#include <optional>

#include <util/types.hpp>

namespace util {

// Simple material structure for glTF models
struct Material {
  Color baseColor = Color(1.0f, 1.0f, 1.0f, 1.0f);
  float metallic = 0.0f;
  float roughness = 0.5f;

  // Texture indices (if loaded)
  int baseColorTextureIndex = -1;
  int metallicRoughnessTextureIndex = -1;
  int normalTextureIndex = -1;

  std::string name;
};

// Model structure containing loaded glTF data
struct Model {
  std::vector<Mesh3D> meshes;
  std::vector<Material> materials;
  std::vector<Texture> textures;
  std::string name;

  [[nodiscard]] bool isValid() const { return !meshes.empty(); }
};

// Load a glTF model from file
std::optional<Model> loadGLTF(const std::string& filepath);

}  // namespace util
