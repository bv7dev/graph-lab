#include <chrono>
#include <print>
#include <string>
#include <thread>
#include <vector>

#include <imgui.h>

#include <gfx/renderer.hpp>
#include <gfx/window.hpp>

#include <util/glm.hpp>
#include <util/gltf_loader.hpp>
#include <util/types.hpp>

int main(int argc, char* argv[]) {
  // Default model path
  std::string modelPath = "assets/models/cube.gltf";

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
  for (const auto& mesh : model.meshes) {
    gpuMeshes.push_back(renderer.uploadMesh(mesh));
  }

  // Camera settings
  float cameraDistance = 5.0f;
  float cameraAngle = 0.0f;
  float cameraHeight = 0.0f;
  float modelRotationY = 0.0f;
  bool autoRotate = true;
  bool showWireframe = false;
  bool showEdges = false;
  bool showPoints = false;
  float pointSize = 5.0f;
  float lineWidth = 1.0f;

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
      ImGui::SliderFloat("Rotation", &modelRotationY, 0.0f, 360.0f);
    }
    ImGui::Separator();

    ImGui::Text("Rendering Options");
    ImGui::Checkbox("Wireframe", &showWireframe);
    ImGui::Checkbox("Show Edges", &showEdges);
    if (showEdges) {
      ImGui::SliderFloat("Line Width", &lineWidth, 0.5f, 5.0f);
    }
    ImGui::Checkbox("Show Points", &showPoints);
    if (showPoints) {
      ImGui::SliderFloat("Point Size", &pointSize, 1.0f, 20.0f);
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
    glm::mat4 view = glm::lookAt(glm::vec3(camX, cameraHeight, camZ),  // Camera position
                                 glm::vec3(0.0f, 0.0f, 0.0f),          // Look at origin
                                 glm::vec3(0.0f, 1.0f, 0.0f));         // Up vector

    // Model transformation
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotationY), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 mvp = projection * view * modelMatrix;

    // Render all meshes
    for (const auto& gpuMesh : gpuMeshes) {
      renderer.drawMesh(gpuMesh, mvp, util::Color(1.0f, 1.0f, 1.0f, 1.0f), showWireframe);

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

  return 0;
}
