#pragma once

#include <cstdint>
#include <memory>
#include <string>

// Forward declare GLFWwindow
struct GLFWwindow;

namespace gfx {

// Template wrapper for compile-time polymorphism
template <typename ImplType>
class WindowTemplate {
 public:
  bool initialize(uint32_t width, uint32_t height, const std::string& title, bool visible = true) {
    return impl.initialize(width, height, title, visible);
  }

  [[nodiscard]] bool shouldClose() const { return impl.shouldClose(); }

  void pollEvents() { impl.pollEvents(); }

  void swapBuffers() { impl.swapBuffers(); }

  [[nodiscard]] uint32_t getWidth() const { return impl.getWidth(); }

  [[nodiscard]] uint32_t getHeight() const { return impl.getHeight(); }

  void setTitle(const std::string& title) { impl.setTitle(title); }

  void setResizable(bool resizable) { impl.setResizable(resizable); }

  void setFullscreen(bool fullscreen) { impl.setFullscreen(fullscreen); }

  [[nodiscard]] GLFWwindow* getNativeWindow() const { return impl.getNativeWindow(); }

 private:
  ImplType impl;
};

// Forward declaration
class WindowGLFW;

// Type aliases
using Window = WindowTemplate<WindowGLFW>;
using WindowPtr = std::unique_ptr<WindowTemplate<WindowGLFW>>;

}  // namespace gfx

// Include the implementation after the template definition
#include "window_glfw.hpp"