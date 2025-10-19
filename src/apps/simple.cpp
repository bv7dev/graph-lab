#include <ratio>
#include <string>
#include <chrono>
#include <thread>

#include <imgui.h>

#include <gfx/renderer.hpp>
#include <gfx/window.hpp>

#include <util/glm.hpp>
#include <util/types.hpp>

int main() {
  gfx::Window window;
  gfx::Renderer renderer;

  util::Mesh3D graphMesh;

  graphMesh.vertices.emplace_back(-1.0f, -1.0f, 0.0f, util::Color(1.0f, 0.0f, 0.0f, 1.0f));  // Bottom-left (red)
  graphMesh.vertices.emplace_back(1.0f, -1.0f, 0.0f, util::Color(0.0f, 1.0f, 0.0f, 1.0f));   // Bottom-right (green)
  graphMesh.vertices.emplace_back(0.0f, 1.0f, 0.0f, util::Color(0.0f, 0.0f, 1.0f, 1.0f));    // Top (blue)
  graphMesh.vertices.emplace_back(1.0f, 1.5f, 0.0f, util::Color(1.0f, 1.0f, 0.0f, 1.0f));    // Extra vertex (yellow)

  graphMesh.addFace(0, 1, 1);

  graphMesh.addEdge(0, 3);

  window.initialize(640, 400, "simple");
  renderer.initialize(window, 640, 400);

  util::MeshGPU graphGPU = renderer.uploadMesh(graphMesh);

  while (!window.shouldClose()) {
    auto start_of_frame = std::chrono::high_resolution_clock::now();
    window.pollEvents();

    renderer.beginFrame();

    ImGui::Begin("Simple App");
    ImGui::Text("Hello, World!");
    ImGui::Text("FPS: %f", renderer.getFramerate());
    ImGui::Text("deltatime: %f ms", ImGui::GetIO().DeltaTime * 1000.0f);
    ImGui::End();

    renderer.clear(util::Color(0.2f, 0.3f, 0.3f, 1.0f));

    // Setup projection matrix
    float aspect = 640.0f / 400.0f;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.01f, 1000.0f);

    // Setup view matrix (camera)
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f),   // Camera position
                                 glm::vec3(0.0f, 0.0f, 0.0f),   // Look at origin
                                 glm::vec3(0.0f, 1.0f, 0.0f));  // Up vector

    auto graphModel = glm::mat4(1.0f);

    graphModel = glm::translate(graphModel, glm::vec3(0.0f, 0.0f, 0.0f));
    graphModel = glm::scale(graphModel, glm::vec3(1.0f, 1.0f, 1.0f));
    graphModel = glm::rotate(graphModel, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 mvp = projection * view * graphModel;

    renderer.drawMesh(graphGPU, mvp, util::Color(1.0f, 1.0f, 1.0f, 1.0f), false);
    renderer.drawMeshEdges(graphGPU, mvp, util::Color(1.0f, 1.0f, 0.0f, 1.0f), 2.0f);

    renderer.drawCircle(100, 200, 20, util::Color(1.0, 1.0, 1.0, 1.0), true);

    renderer.endFrame();
    window.swapBuffers();

    auto end_of_frame = std::chrono::high_resolution_clock::now();
    auto frame_time = end_of_frame - start_of_frame;
    std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(16.666666f) - frame_time);
  }

  renderer.freeMesh(graphGPU);
}
