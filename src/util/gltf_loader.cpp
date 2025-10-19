#include <util/gltf_loader.hpp>

#include <print>
#include <cstdint>

// Disable warnings for tinygltf and its embedded STB libraries
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#pragma GCC diagnostic pop

namespace util {

std::optional<Model> loadGLTF(const std::string& filepath) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool ret = false;

  // Check file extension
  if (filepath.ends_with(".glb")) {
    ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, filepath);
  } else if (filepath.ends_with(".gltf")) {
    ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath);
  } else {
    std::println(stderr, "Unknown glTF file extension: {}", filepath);
    return std::nullopt;
  }

  if (!warn.empty()) {
    std::println("glTF Warning: {}", warn);
  }

  if (!err.empty()) {
    std::println(stderr, "glTF Error: {}", err);
  }

  if (!ret) {
    std::println(stderr, "Failed to load glTF file: {}", filepath);
    return std::nullopt;
  }

  Model model;
  model.name = filepath;

  // Load materials
  for (const auto& gltfMat : gltfModel.materials) {
    Material mat;
    mat.name = gltfMat.name;

    // PBR metallic-roughness
    if (gltfMat.pbrMetallicRoughness.baseColorFactor.size() >= 4) {
      mat.baseColor = Color(static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[0]),
                            static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[1]),
                            static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[2]),
                            static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[3]));
    }

    mat.metallic = static_cast<float>(gltfMat.pbrMetallicRoughness.metallicFactor);
    mat.roughness = static_cast<float>(gltfMat.pbrMetallicRoughness.roughnessFactor);

    mat.baseColorTextureIndex = gltfMat.pbrMetallicRoughness.baseColorTexture.index;
    mat.metallicRoughnessTextureIndex = gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index;
    mat.normalTextureIndex = gltfMat.normalTexture.index;

    model.materials.push_back(mat);
  }

  // Load meshes
  for (const auto& gltfMesh : gltfModel.meshes) {
    for (const auto& primitive : gltfMesh.primitives) {
      Mesh3D mesh;

      // Get position accessor
      const auto& posAccessor = gltfModel.accessors[primitive.attributes.at("POSITION")];
      const auto& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
      const auto& posBuffer = gltfModel.buffers[posBufferView.buffer];

      // Get position data
      const float* positions =
          reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

      // Get color data if available
      const float* colors = nullptr;
      bool hasColors = false;
      if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
        const auto& colorAccessor = gltfModel.accessors[primitive.attributes.at("COLOR_0")];
        const auto& colorBufferView = gltfModel.bufferViews[colorAccessor.bufferView];
        const auto& colorBuffer = gltfModel.buffers[colorBufferView.buffer];
        colors =
            reinterpret_cast<const float*>(&colorBuffer.data[colorBufferView.byteOffset + colorAccessor.byteOffset]);
        hasColors = true;
      }

      // Get material color
      Color materialColor(1.0f, 1.0f, 1.0f, 1.0f);
      if (primitive.material >= 0 && primitive.material < static_cast<int>(model.materials.size())) {
        materialColor = model.materials[primitive.material].baseColor;
      }

      // Load vertices
      for (size_t i = 0; i < posAccessor.count; ++i) {
        glm::vec3 pos(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);

        Color color = materialColor;
        if (hasColors) {
          color = Color(colors[i * 4 + 0], colors[i * 4 + 1], colors[i * 4 + 2], colors[i * 4 + 3]);
        }

        mesh.vertices.emplace_back(pos, color);
      }

      // Load indices
      if (primitive.indices >= 0) {
        const auto& indexAccessor = gltfModel.accessors[primitive.indices];
        const auto& indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
        const auto& indexBuffer = gltfModel.buffers[indexBufferView.buffer];

        const uint8_t* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

        // Handle different index types
        for (size_t i = 0; i < indexAccessor.count; ++i) {
          uint32_t index = 0;

          if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            index = reinterpret_cast<const uint16_t*>(indexData)[i];
          } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            index = reinterpret_cast<const uint32_t*>(indexData)[i];
          } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            index = indexData[i];
          }

          mesh.faces.push_back(index);
        }
      } else {
        // No indices - generate sequential indices
        for (size_t i = 0; i < posAccessor.count; ++i) {
          mesh.faces.push_back(static_cast<uint32_t>(i));
        }
      }

      model.meshes.push_back(mesh);
    }
  }

  if (model.meshes.empty()) {
    std::println(stderr, "No meshes found in glTF file: {}", filepath);
    return std::nullopt;
  }

  std::println("Loaded glTF: {} with {} mesh(es)", filepath, model.meshes.size());

  return model;
}

}  // namespace util
