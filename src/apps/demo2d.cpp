#include <cmath>
#include <cstdint>
#include <numbers>
#include <print>

#include <imgui.h>

#include <gfx/renderer.hpp>
#include <gfx/window.hpp>

#include <util/glm.hpp>
#include <util/types.hpp>

// Helper to convert Mesh2D to Mesh3D (z=0) for GPU upload
util::Mesh3D toMesh3D(const util::Mesh2D& mesh2D) {
  util::Mesh3D mesh3D;
  for (const auto& v : mesh2D.vertices) {
    mesh3D.vertices.emplace_back(v.position.x, v.position.y, 0.0f, v.color);
  }
  mesh3D.faces = mesh2D.faces;
  return mesh3D;
}

int main() {
  gfx::Window window;
  gfx::Renderer renderer;

  if (!window.initialize(800, 600, "2D Mesh Renderer Demo")) {
    std::print("Failed to initialize GLFW window!\n");
    return -1;
  }

  if (!renderer.initialize(window, 800, 600)) {
    std::print("Failed to initialize renderer!\n");
    return -1;
  }

  // UI state variables
  float houseTint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float starTint[4] = {0.8f, 1.0f, 1.0f, 1.0f};
  float bgColor[4] = {0.1f, 0.1f, 0.3f, 1.0f};
  float houseScale = 1.0f;
  float houseRotation = 0.0f;
  float starScale = 1.0f;
  float starRotation = 0.0f;

  // Create a simple custom mesh by hand: a colorful house shape
  util::Mesh2D houseMesh;

  // Define vertices with positions and colors (now with bottom-left origin)
  // House base (square)
  houseMesh.vertices.emplace_back(100.0f, 200.0f, util::Color(1.0f, 0.0f, 0.0f, 1.0f));  // 0: bottom-left (red)
  houseMesh.vertices.emplace_back(300.0f, 200.0f, util::Color(0.0f, 1.0f, 0.0f, 1.0f));  // 1: bottom-right (green)
  houseMesh.vertices.emplace_back(300.0f, 400.0f, util::Color(0.0f, 0.0f, 1.0f, 1.0f));  // 2: top-right (blue)
  houseMesh.vertices.emplace_back(100.0f, 400.0f, util::Color(1.0f, 1.0f, 0.0f, 1.0f));  // 3: top-left (yellow)

  // Roof (triangle)
  houseMesh.vertices.emplace_back(200.0f, 500.0f, util::Color(1.0f, 0.0f, 1.0f, 1.0f));  // 4: roof peak (magenta)

  // Door
  houseMesh.vertices.emplace_back(160.0f, 200.0f, util::Color(0.6f, 0.3f, 0.0f, 1.0f));  // 5: door bottom-left (brown)
  houseMesh.vertices.emplace_back(240.0f, 200.0f, util::Color(0.6f, 0.3f, 0.0f, 1.0f));  // 6: door bottom-right (brown)
  houseMesh.vertices.emplace_back(240.0f, 320.0f,
                                  util::Color(0.4f, 0.2f, 0.0f, 1.0f));  // 7: door top-right (dark brown)
  houseMesh.vertices.emplace_back(160.0f, 320.0f,
                                  util::Color(0.4f, 0.2f, 0.0f, 1.0f));  // 8: door top-left (dark brown)

  // Define faces (triangles) - house base and roof
  houseMesh.addFace(0, 1, 2);  // Base triangle 1
  houseMesh.addFace(0, 2, 3);  // Base triangle 2
  houseMesh.addFace(3, 2, 4);  // Roof triangle

  // Door faces
  houseMesh.addFace(5, 6, 7);  // Door triangle 1
  houseMesh.addFace(5, 7, 8);  // Door triangle 2

  // Create a second mesh: a simple star
  util::Mesh2D starMesh;

  float centerX = 550.0f;
  float centerY = 300.0f;
  float outerRadius = 80.0f;
  float innerRadius = 30.0f;

  // Star vertices (5-pointed star)
  for (int i = 0; i < 10; ++i) {
    float angle = (static_cast<float>(i) * std::numbers::pi_v<float> / 5.0f) - (std::numbers::pi_v<float> / 2.0f);
    float radius = (i % 2 == 0) ? outerRadius : innerRadius;
    float x = centerX + (radius * cosf(angle));
    float y = centerY + (radius * sinf(angle));

    // Rainbow colors
    float hue = static_cast<float>(i) / 10.0f;
    util::Color color(hue, 1.0f - hue, 0.5f, 1.0f);
    starMesh.vertices.emplace_back(x, y, color);
  }

  // Star center vertex
  starMesh.vertices.emplace_back(centerX, centerY, util::Color(1.0f, 1.0f, 1.0f, 1.0f));

  // Star faces (triangles from center)
  for (uint32_t i = 0; i < 10; ++i) {
    starMesh.addFace(10, i, (i + 1) % 10);
  }

  // Upload meshes to GPU once (efficient!)
  util::MeshGPU houseGPU = renderer.uploadMesh(toMesh3D(houseMesh));
  util::MeshGPU starGPU = renderer.uploadMesh(toMesh3D(starMesh));

  // Orthographic projection for 2D (matches screen coordinates)
  glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);

  // Simple rendering loop
  while (!window.shouldClose()) {
    window.pollEvents();

    renderer.beginFrame();

    // Create ImGui control panel
    ImGui::Begin("Mesh Renderer Controls");
    ImGui::Text("Application: %.1f FPS", renderer.getFramerate());
    ImGui::Separator();

    ImGui::Text("Background");
    ImGui::ColorEdit3("BG Color", bgColor);
    ImGui::Separator();

    ImGui::Text("House Mesh");
    ImGui::ColorEdit4("House Tint", houseTint);
    ImGui::SliderFloat("House Scale", &houseScale, 0.5f, 2.0f);
    ImGui::SliderFloat("House Rotation", &houseRotation, 0.0f, 360.0f);
    ImGui::Separator();

    ImGui::Text("Star Mesh");
    ImGui::ColorEdit4("Star Tint", starTint);
    ImGui::SliderFloat("Star Scale", &starScale, 0.5f, 2.0f);
    ImGui::SliderFloat("Star Rotation", &starRotation, 0.0f, 360.0f);

    ImGui::End();

    // Clear screen with adjustable color
    renderer.clear(util::Color(bgColor[0], bgColor[1], bgColor[2], bgColor[3]));

    // Draw house with GPU-side transformations (efficient!)
    auto houseModel = glm::mat4(1.0f);
    houseModel = glm::translate(houseModel, glm::vec3(200.0f, 300.0f, 0.0f));
    houseModel = glm::rotate(houseModel, glm::radians(houseRotation), glm::vec3(0.0f, 0.0f, 1.0f));
    houseModel = glm::scale(houseModel, glm::vec3(houseScale, houseScale, 1.0f));
    houseModel = glm::translate(houseModel, glm::vec3(-200.0f, -300.0f, 0.0f));

    glm::mat4 houseMVP = projection * houseModel;
    renderer.drawMesh(houseGPU, houseMVP, util::Color(houseTint[0], houseTint[1], houseTint[2], houseTint[3]));

    // Draw star with GPU-side transformations (efficient!)
    auto starModel = glm::mat4(1.0f);
    starModel = glm::translate(starModel, glm::vec3(centerX, centerY, 0.0f));
    starModel = glm::rotate(starModel, glm::radians(starRotation), glm::vec3(0.0f, 0.0f, 1.0f));
    starModel = glm::scale(starModel, glm::vec3(starScale, starScale, 1.0f));
    starModel = glm::translate(starModel, glm::vec3(-centerX, -centerY, 0.0f));

    glm::mat4 starMVP = projection * starModel;
    renderer.drawMesh(starGPU, starMVP, util::Color(starTint[0], starTint[1], starTint[2], starTint[3]));

    // Draw some additional test shapes
    renderer.drawCircle(650, 100, 30, util::Color(1.0f, 1.0f, 0.0f, 1.0f), false);
    renderer.drawRectangle(50, 550, 100, 50, util::Color(1.0f, 0.5f, 0.0f, 1.0f), false);

    renderer.drawLine(0.0f, 0.0f, 800.0f, 600.0f, util::Color(1.0f, 1.0f, 1.0f, 0.6f));

    renderer.endFrame();

    window.swapBuffers();
  }

  // Cleanup GPU resources
  renderer.freeMesh(houseGPU);
  renderer.freeMesh(starGPU);
}
