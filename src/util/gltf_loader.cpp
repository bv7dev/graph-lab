#include <optional>
#include <cstddef>
#include <cstdint>
#include <string>
#include <print>

#include <util/gltf_loader.hpp>
#include <util/types.hpp>
#include <util/glm.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

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

  // Load textures
  for (const auto& gltfTexture : gltfModel.textures) {
    if (gltfTexture.source < 0 || gltfTexture.source >= static_cast<int>(gltfModel.images.size())) {
      continue;
    }

    const auto& gltfImage = gltfModel.images[gltfTexture.source];

    Texture texture;
    texture.width = gltfImage.width;
    texture.height = gltfImage.height;
    texture.channels = gltfImage.component;
    texture.data = gltfImage.image;  // Copy image data

    model.textures.push_back(texture);
  }

  std::println("Loaded {} texture(s)", model.textures.size());

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
      mesh.materialIndex = primitive.material;

      // Get position accessor
      const auto& posAccessor = gltfModel.accessors[primitive.attributes.at("POSITION")];
      const auto& posBufferView = gltfModel.bufferViews[posAccessor.bufferView];
      const auto& posBuffer = gltfModel.buffers[posBufferView.buffer];

      // Get position data
      const auto* positions =
          reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);

      // Get normal data if available
      const float* normals = nullptr;
      bool hasNormals = false;
      if (primitive.attributes.contains("NORMAL")) {
        const auto& normalAccessor = gltfModel.accessors[primitive.attributes.at("NORMAL")];
        const auto& normalBufferView = gltfModel.bufferViews[normalAccessor.bufferView];
        const auto& normalBuffer = gltfModel.buffers[normalBufferView.buffer];
        normals =
            reinterpret_cast<const float*>(&normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset]);
        hasNormals = true;
      }

      // Get texture coordinate data if available
      const float* texCoords = nullptr;
      bool hasTexCoords = false;
      if (primitive.attributes.contains("TEXCOORD_0")) {
        const auto& texCoordAccessor = gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")];
        const auto& texCoordBufferView = gltfModel.bufferViews[texCoordAccessor.bufferView];
        const auto& texCoordBuffer = gltfModel.buffers[texCoordBufferView.buffer];
        texCoords = reinterpret_cast<const float*>(
            &texCoordBuffer.data[texCoordBufferView.byteOffset + texCoordAccessor.byteOffset]);
        hasTexCoords = true;
      }

      // Get color data if available
      const float* colors = nullptr;
      bool hasColors = false;
      if (primitive.attributes.contains("COLOR_0")) {
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
        Vertex3D vertex;

        // Position
        vertex.position = glm::vec3(positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2]);

        // Normal
        if (hasNormals) {
          vertex.normal = glm::vec3(normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]);
        }

        // Texture coordinates
        if (hasTexCoords) {
          vertex.texCoord = glm::vec2(texCoords[i * 2 + 0], texCoords[i * 2 + 1]);
        }

        // Color
        if (hasColors) {
          vertex.color = Color(colors[i * 4 + 0], colors[i * 4 + 1], colors[i * 4 + 2], colors[i * 4 + 3]);
        } else {
          vertex.color = materialColor;
        }

        mesh.vertices.push_back(vertex);
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
