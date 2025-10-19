#pragma once

#include <cstdint>
#include <string>

#include <GLFW/glfw3.h>

namespace gfx {

class WindowGLFW {
 public:
  WindowGLFW();
  ~WindowGLFW();

  WindowGLFW(const WindowGLFW&) = delete;
  WindowGLFW(WindowGLFW&&) = delete;
  WindowGLFW& operator=(const WindowGLFW&) = delete;
  WindowGLFW& operator=(WindowGLFW&&) = delete;

  bool initialize(uint32_t width, uint32_t height, const std::string& title, bool visible = true);
  [[nodiscard]] bool shouldClose() const;
  void pollEvents();
  void swapBuffers();
  [[nodiscard]] uint32_t getWidth() const;
  [[nodiscard]] uint32_t getHeight() const;
  void setTitle(const std::string& title);
  void setResizable(bool resizable);
  void setFullscreen(bool fullscreen);
  [[nodiscard]] GLFWwindow* getNativeWindow() const;

 private:
  GLFWwindow* m_window;
  uint32_t m_width;
  uint32_t m_height;
  bool m_fullscreen;
  bool m_initialized;

  void cleanup();
};

}  // namespace gfx
