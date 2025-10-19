#include <cmath>
#include <print>
#include <ratio>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <cstddef>

#include <imgui.h>

#include <gfx/renderer.hpp>
#include <gfx/window.hpp>

#include <util/glm.hpp>
#include <util/gltf_loader.hpp>
#include <util/types.hpp>

int main(int argc, char** argv) {
  // Default model path
  std::string modelPath = "assets/models/DamagedHelmet.glb";

  // Allow specifying model path as command line argument
  if (argc > 1) {
    modelPath = argv[1];
  }

  // Load glTF model
  auto modelOpt = util::loadGLTF(modelPath);
  if (!modelOpt.has_value()) {
    std::println(stderr, "Failed to load model: {}", modelPath);
    std::println(stderr, "Usage: {} [path/to/model.gltf]", argv[0]);
    return 1;
  }

  util::Model& model = modelOpt.value();
  std::println("Successfully loaded model with {} mesh(es)", model.meshes.size());

  // Initialize window and renderer
  gfx::Window window;
  gfx::Renderer renderer;

  window.initialize(800, 600, "glTF Viewer - " + model.name);
  renderer.initialize(window, 800, 600);

  // Upload all meshes to GPU
  std::vector<util::MeshGPU> gpuMeshes;
  gpuMeshes.reserve(model.meshes.size());
  for (const auto& mesh : model.meshes) {
    gpuMeshes.push_back(renderer.uploadMesh(mesh));
  }

  // Upload all textures to GPU
  std::vector<util::TextureGPU> gpuTextures;
  gpuTextures.reserve(model.textures.size());
  for (const auto& texture : model.textures) {
    gpuTextures.push_back(renderer.uploadTexture(texture));
  }

  std::println("Uploaded {} texture(s) to GPU", gpuTextures.size());

  // Camera settings
  float cameraDistance = 5.0f;
  float cameraAngle = 0.0f;
  float cameraHeight = 0.0f;
  float modelRotationY = 0.0f;
  float modelRotationX = 0.0f;  // For pitch adjustment
  bool autoRotate = true;
  bool showWireframe = false;
  bool showEdges = false;
  bool showPoints = false;
  bool usePBR = true;
  float pointSize = 5.0f;
  float lineWidth = 1.0f;

  // Lighting settings
  float lightDistance = 10.0f;
  float lightAngle = 45.0f;
  float lightHeight = 5.0f;
  float lightIntensity = 5.0f;  // Brighter lighting
  util::Color lightColor(1.0f, 1.0f, 1.0f, 1.0f);

  while (!window.shouldClose()) {
    auto start_of_frame = std::chrono::high_resolution_clock::now();
    window.pollEvents();

    renderer.beginFrame();

    // UI
    ImGui::Begin("glTF Viewer");
    ImGui::Text("Model: %s", model.name.c_str());
    ImGui::Text("Meshes: %zu", model.meshes.size());
    ImGui::Text("FPS: %.1f", renderer.getFramerate());
    ImGui::Separator();

    ImGui::Text("Camera Controls");
    ImGui::SliderFloat("Distance", &cameraDistance, 1.0f, 20.0f);
    ImGui::SliderFloat("Angle", &cameraAngle, 0.0f, 360.0f);
    ImGui::SliderFloat("Height", &cameraHeight, -5.0f, 5.0f);
    ImGui::Separator();

    ImGui::Text("Model Controls");
    ImGui::Checkbox("Auto Rotate", &autoRotate);
    if (!autoRotate) {
      ImGui::SliderFloat("Rotation Y", &modelRotationY, 0.0f, 360.0f);
    }
    ImGui::SliderFloat("Pitch", &modelRotationX, -180.0f, 180.0f);
    ImGui::Separator();

    ImGui::Text("Rendering Options");
    ImGui::Checkbox("PBR Lighting", &usePBR);
    ImGui::Checkbox("Wireframe", &showWireframe);
    ImGui::Checkbox("Show Edges", &showEdges);
    if (showEdges) {
      ImGui::SliderFloat("Line Width", &lineWidth, 0.5f, 5.0f);
    }
    ImGui::Checkbox("Show Points", &showPoints);
    if (showPoints) {
      ImGui::SliderFloat("Point Size", &pointSize, 1.0f, 20.0f);
    }
    ImGui::Separator();

    if (usePBR) {
      ImGui::Text("Lighting");
      ImGui::SliderFloat("Light Distance", &lightDistance, 5.0f, 50.0f);
      ImGui::SliderFloat("Light Angle", &lightAngle, 0.0f, 360.0f);
      ImGui::SliderFloat("Light Height", &lightHeight, -10.0f, 10.0f);
      ImGui::SliderFloat("Light Intensity", &lightIntensity, 0.5f, 200.0f);
      ImGui::ColorEdit3("Light Color", &lightColor.r);
    }

    ImGui::End();

    // Auto-rotate model
    if (autoRotate) {
      modelRotationY += ImGui::GetIO().DeltaTime * 30.0f;  // 30 degrees per second
      if (modelRotationY > 360.0f) {
        modelRotationY -= 360.0f;
      }
    }

    renderer.clear(util::Color(0.1f, 0.1f, 0.15f, 1.0f));

    // Setup projection matrix
    float aspect = 800.0f / 600.0f;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    // Setup view matrix (camera)
    float camX = cameraDistance * std::sin(glm::radians(cameraAngle));
    float camZ = cameraDistance * std::cos(glm::radians(cameraAngle));
    glm::vec3 cameraPos(camX, cameraHeight, camZ);
    glm::mat4 view = glm::lookAt(cameraPos,                     // Camera position
                                 glm::vec3(0.0f, 0.0f, 0.0f),   // Look at origin
                                 glm::vec3(0.0f, 1.0f, 0.0f));  // Up vector

    // Model transformation
    auto modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotationX), glm::vec3(1.0f, 0.0f, 0.0f));  // Pitch
    modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));  // Yaw

    // Light position
    float lightX = lightDistance * std::sin(glm::radians(lightAngle));
    float lightZ = lightDistance * std::cos(glm::radians(lightAngle));
    glm::vec3 lightPos(lightX, lightHeight, lightZ);

    glm::mat4 mvp = projection * view * modelMatrix;

    // Render all meshes
    for (size_t i = 0; i < gpuMeshes.size(); ++i) {
      const auto& gpuMesh = gpuMeshes[i];
      const auto& mesh = model.meshes[i];

      if (usePBR) {
        // Setup PBR material
        util::PBRMaterial material;

        // Get material from mesh
        if (mesh.materialIndex >= 0 && mesh.materialIndex < static_cast<int>(model.materials.size())) {
          const auto& srcMat = model.materials[mesh.materialIndex];
          material.baseColor = srcMat.baseColor;
          material.metallic = srcMat.metallic;
          material.roughness = srcMat.roughness;

          // Map texture indices to GPU texture IDs
          if (srcMat.baseColorTextureIndex >= 0 &&
              srcMat.baseColorTextureIndex < static_cast<int>(gpuTextures.size())) {
            material.baseColorTexture = gpuTextures[srcMat.baseColorTextureIndex].id;
          }
          if (srcMat.metallicRoughnessTextureIndex >= 0 &&
              srcMat.metallicRoughnessTextureIndex < static_cast<int>(gpuTextures.size())) {
            material.metallicRoughnessTexture = gpuTextures[srcMat.metallicRoughnessTextureIndex].id;
          }
          if (srcMat.normalTextureIndex >= 0 && srcMat.normalTextureIndex < static_cast<int>(gpuTextures.size())) {
            material.normalTexture = gpuTextures[srcMat.normalTextureIndex].id;
          }
        }

        // Scale light color by intensity
        util::Color intensifiedLight = lightColor * lightIntensity;
        renderer.drawMeshPBR(gpuMesh, modelMatrix, view, projection, material, cameraPos, lightPos, intensifiedLight);
      } else {
        renderer.drawMesh(gpuMesh, mvp, util::Color(1.0f, 1.0f, 1.0f, 1.0f), showWireframe);
      }

      if (showEdges) {
        renderer.drawMeshEdges(gpuMesh, mvp, util::Color(0.0f, 1.0f, 1.0f, 1.0f), lineWidth);
      }

      if (showPoints) {
        renderer.drawMeshPoints(gpuMesh, mvp, util::Color(1.0f, 1.0f, 0.0f, 1.0f), pointSize);
      }
    }

    renderer.endFrame();
    window.swapBuffers();

    auto end_of_frame = std::chrono::high_resolution_clock::now();
    auto frame_time = end_of_frame - start_of_frame;
    std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(16.666666f) - frame_time);
  }

  // Cleanup
  for (auto& gpuMesh : gpuMeshes) {
    renderer.freeMesh(gpuMesh);
  }
  for (auto& gpuTexture : gpuTextures) {
    renderer.freeTexture(gpuTexture);
  }

  return 0;
}
