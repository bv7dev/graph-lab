#include <cmath>
#include <numbers>
#include <print>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <gfx/renderer.hpp>
#include <gfx/window.hpp>

#include <util/glm.hpp>
#include <util/types.hpp>

// Create a colorful cube mesh
util::Mesh3D createCube(float size) {
  util::Mesh3D cube;

  float h = size / 2.0f;

  // Define 8 vertices of the cube with different colors
  cube.vertices.emplace_back(-h, -h, -h, util::Color(1.0f, 0.0f, 0.0f, 1.0f));  // 0: front-bottom-left (red)
  cube.vertices.emplace_back(h, -h, -h, util::Color(0.0f, 1.0f, 0.0f, 1.0f));   // 1: front-bottom-right (green)
  cube.vertices.emplace_back(h, h, -h, util::Color(0.0f, 0.0f, 1.0f, 1.0f));    // 2: front-top-right (blue)
  cube.vertices.emplace_back(-h, h, -h, util::Color(1.0f, 1.0f, 0.0f, 1.0f));   // 3: front-top-left (yellow)
  cube.vertices.emplace_back(-h, -h, h, util::Color(1.0f, 0.0f, 1.0f, 1.0f));   // 4: back-bottom-left (magenta)
  cube.vertices.emplace_back(h, -h, h, util::Color(0.0f, 1.0f, 1.0f, 1.0f));    // 5: back-bottom-right (cyan)
  cube.vertices.emplace_back(h, h, h, util::Color(1.0f, 1.0f, 1.0f, 1.0f));     // 6: back-top-right (white)
  cube.vertices.emplace_back(-h, h, h, util::Color(0.5f, 0.5f, 0.5f, 1.0f));    // 7: back-top-left (gray)

  // Define 12 triangular faces (2 per cube face)
  // Using CCW winding for outward-pointing normals (standard OpenGL convention)
  // Vertices: 0=(-,-,-), 1=(+,-,-), 2=(+,+,-), 3=(-,+,-), 4=(-,-,+), 5=(+,-,+), 6=(+,+,+), 7=(-,+,+)

  // Front face (z = -h): normal toward -Z
  cube.addFace(0, 3, 2);
  cube.addFace(0, 2, 1);

  // Back face (z = +h): normal toward +Z
  cube.addFace(4, 5, 6);
  cube.addFace(4, 6, 7);

  // Top face (y = +h): normal toward +Y
  cube.addFace(3, 7, 6);
  cube.addFace(3, 6, 2);

  // Bottom face (y = -h): normal toward -Y
  cube.addFace(0, 1, 5);
  cube.addFace(0, 5, 4);

  // Right face (x = +h): normal toward +X
  cube.addFace(1, 2, 6);
  cube.addFace(1, 6, 5);

  // Left face (x = -h): normal toward -X
  cube.addFace(4, 7, 3);
  cube.addFace(4, 3, 0);

  return cube;
}

// Create a pyramid mesh
util::Mesh3D createPyramid(float size) {
  util::Mesh3D pyramid;

  float h = size / 2.0f;

  // Define 5 vertices
  pyramid.vertices.emplace_back(-h, -h, -h, util::Color(1.0f, 0.0f, 0.0f, 1.0f));     // 0: base front-left (red)
  pyramid.vertices.emplace_back(h, -h, -h, util::Color(0.0f, 1.0f, 0.0f, 1.0f));      // 1: base front-right (green)
  pyramid.vertices.emplace_back(h, -h, h, util::Color(0.0f, 0.0f, 1.0f, 1.0f));       // 2: base back-right (blue)
  pyramid.vertices.emplace_back(-h, -h, h, util::Color(1.0f, 1.0f, 0.0f, 1.0f));      // 3: base back-left (yellow)
  pyramid.vertices.emplace_back(0.0f, h, 0.0f, util::Color(1.0f, 0.0f, 1.0f, 1.0f));  // 4: apex (magenta)

  // Define faces with CCW winding for outward-pointing normals
  // Vertices: 0=(-,-,-), 1=(+,-,-), 2=(+,-,+), 3=(-,-,+), 4=(0,+,0)

  // Base (y = -h): normal toward -Y (downward)
  pyramid.addFace(0, 1, 2);
  pyramid.addFace(0, 2, 3);

  // Side faces: normals point outward and upward from pyramid
  // Front face (z = -h side): normal toward -Z and +Y
  pyramid.addFace(0, 4, 1);
  // Right face (x = +h side): normal toward +X and +Y
  pyramid.addFace(1, 4, 2);
  // Back face (z = +h side): normal toward +Z and +Y
  pyramid.addFace(2, 4, 3);
  // Left face (x = -h side): normal toward -X and +Y
  pyramid.addFace(3, 4, 0);

  return pyramid;
}

int main() {
  gfx::Window window;
  gfx::Renderer renderer;

  if (!window.initialize(800, 600, "3D Mesh Renderer Demo")) {
    std::print("Failed to initialize GLFW window!\n");
    return -1;
  }

  if (!renderer.initialize(window, 800, 600)) {
    std::print("Failed to initialize renderer!\n");
    return -1;
  }

  // UI state variables
  float bgColor[4] = {0.05f, 0.05f, 0.15f, 1.0f};
  float cubeTint[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float pyramidTint[4] = {1.0f, 1.0f, 1.0f, 1.0f};

  float cubeRotationX = 0.4f;
  float cubeRotationY = 0.4f;
  float cubeRotationZ = 0.0f;

  float pyramidRotationX = 0.2f;
  float pyramidRotationY = 0.8f;
  float pyramidRotationZ = 0.0f;

  float fov = 60.0f;
  float cameraDistance = 5.0f;

  bool autoRotate = true;
  float autoRotateSpeed = 0.5f;

  // Create 3D meshes
  util::Mesh3D cubeMesh = createCube(1.5f);
  util::Mesh3D pyramidMesh = createPyramid(1.5f);

  // Upload meshes to GPU once (efficient - reuse every frame)
  util::MeshGPU cubeGPU = renderer.uploadMesh(cubeMesh);
  util::MeshGPU pyramidGPU = renderer.uploadMesh(pyramidMesh);

  // Rendering loop
  while (!window.shouldClose()) {
    window.pollEvents();

    renderer.beginFrame();

    // Create ImGui control panel
    ImGui::Begin("3D Demo Controls");
    ImGui::Text("FPS: %.1f", renderer.getFramerate());
    ImGui::Separator();

    ImGui::Text("Camera");
    ImGui::SliderFloat("FOV", &fov, 30.0f, 120.0f);
    ImGui::SliderFloat("Distance", &cameraDistance, 2.0f, 10.0f);
    ImGui::Checkbox("Auto Rotate", &autoRotate);
    if (autoRotate) {
      ImGui::SliderFloat("Rotation Speed", &autoRotateSpeed, 0.1f, 2.0f);
    }
    ImGui::Separator();

    ImGui::Text("Background");
    ImGui::ColorEdit3("BG Color", bgColor);
    ImGui::Separator();

    ImGui::Text("Cube (Left)");
    ImGui::ColorEdit4("Cube Tint", cubeTint);
    ImGui::SliderFloat("Cube Rot X", &cubeRotationX, 0.0f, 2.0f * std::numbers::pi_v<float>);
    ImGui::SliderFloat("Cube Rot Y", &cubeRotationY, 0.0f, 2.0f * std::numbers::pi_v<float>);
    ImGui::SliderFloat("Cube Rot Z", &cubeRotationZ, 0.0f, 2.0f * std::numbers::pi_v<float>);
    ImGui::Separator();

    ImGui::Text("Pyramid (Right)");
    ImGui::ColorEdit4("Pyramid Tint", pyramidTint);
    ImGui::SliderFloat("Pyramid Rot X", &pyramidRotationX, 0.0f, 2.0f * std::numbers::pi_v<float>);
    ImGui::SliderFloat("Pyramid Rot Y", &pyramidRotationY, 0.0f, 2.0f * std::numbers::pi_v<float>);
    ImGui::SliderFloat("Pyramid Rot Z", &pyramidRotationZ, 0.0f, 2.0f * std::numbers::pi_v<float>);

    ImGui::End();

    // Auto-rotation
    if (autoRotate) {
      float deltaTime = 1.0f / 60.0f;  // Approximate
      cubeRotationY += deltaTime * autoRotateSpeed;
      pyramidRotationY += deltaTime * autoRotateSpeed;
    }

    // Clear screen
    renderer.clear(util::Color(bgColor[0], bgColor[1], bgColor[2], bgColor[3]));

    // Setup projection matrix
    float aspect = 800.0f / 600.0f;
    glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);

    // Setup view matrix (camera)
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, cameraDistance),  // Camera position
                                 glm::vec3(0.0f, 0.0f, 0.0f),            // Look at origin
                                 glm::vec3(0.0f, 1.0f, 0.0f));           // Up vector

    // Render cube (positioned on the left)
    auto cubeModel = glm::mat4(1.0f);
    cubeModel = glm::translate(cubeModel, glm::vec3(-1.5f, 0.0f, 0.0f));  // Move left
    cubeModel = glm::rotate(cubeModel, cubeRotationX, glm::vec3(1.0f, 0.0f, 0.0f));
    cubeModel = glm::rotate(cubeModel, cubeRotationY, glm::vec3(0.0f, 1.0f, 0.0f));
    cubeModel = glm::rotate(cubeModel, cubeRotationZ, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 cubeMVP = projection * view * cubeModel;
    renderer.drawMesh(cubeGPU, cubeMVP, util::Color(cubeTint[0], cubeTint[1], cubeTint[2], cubeTint[3]));

    // Render pyramid (positioned on the right)
    auto pyramidModel = glm::mat4(1.0f);
    pyramidModel = glm::translate(pyramidModel, glm::vec3(1.5f, 0.0f, 0.0f));  // Move right
    pyramidModel = glm::rotate(pyramidModel, pyramidRotationX, glm::vec3(1.0f, 0.0f, 0.0f));
    pyramidModel = glm::rotate(pyramidModel, pyramidRotationY, glm::vec3(0.0f, 1.0f, 0.0f));
    pyramidModel = glm::rotate(pyramidModel, pyramidRotationZ, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::mat4 pyramidMVP = projection * view * pyramidModel;
    renderer.drawMesh(pyramidGPU, pyramidMVP,
                      util::Color(pyramidTint[0], pyramidTint[1], pyramidTint[2], pyramidTint[3]));

    renderer.drawLine(0.0f, 0.0f, 800.0f, 600.0f, util::Color(1.0f, 1.0f, 1.0f, 0.2f));

    renderer.endFrame();

    window.swapBuffers();
  }

  // Cleanup GPU resources
  renderer.freeMesh(cubeGPU);
  renderer.freeMesh(pyramidGPU);
}
