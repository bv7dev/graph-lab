#include <cstdint>
#include <string>

#include <GLFW/glfw3.h>

#include <gfx/window_glfw.hpp>

namespace gfx {

WindowGLFW::WindowGLFW()
    : m_window(nullptr),
      m_width(0),
      m_height(0),
      m_fullscreen(false),
      m_initialized(false),
      m_api(GraphicsAPI::OpenGL) {}

WindowGLFW::~WindowGLFW() { cleanup(); }

auto WindowGLFW::initialize(uint32_t width, uint32_t height, const std::string& title, bool visible) -> bool {
  return initializeWithAPI(width, height, title, GraphicsAPI::OpenGL, visible);  // Use OpenGL for WSL2
}

auto WindowGLFW::initializeWithAPI(uint32_t width, uint32_t height, const std::string& title, GraphicsAPI api,
                                   bool visible) -> bool {
  if (m_initialized) {
    return false;
  }

  if (glfwInit() == GLFW_FALSE) {
    return false;
  }

  m_api = api;

  if (api == GraphicsAPI::OpenGL) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  } else {
    // Vulkan - tell GLFW not to create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  }

  glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

  m_window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
  if (m_window == nullptr) {
    glfwTerminate();
    return false;
  }

  if (api == GraphicsAPI::OpenGL) {
    glfwMakeContextCurrent(m_window);
    // Enable V-Sync
    glfwSwapInterval(1);
  }

  glfwSetWindowUserPointer(m_window, this);

  m_width = width;
  m_height = height;
  m_initialized = true;

  return true;
}

bool WindowGLFW::shouldClose() const { return m_window != nullptr ? glfwWindowShouldClose(m_window) != 0 : true; }

void WindowGLFW::pollEvents() {
  if (m_window != nullptr) {
    glfwPollEvents();
  }
}

void WindowGLFW::swapBuffers() {
  if (m_window != nullptr) {
    glfwSwapBuffers(m_window);
  }
}

uint32_t WindowGLFW::getWidth() const { return m_width; }

uint32_t WindowGLFW::getHeight() const { return m_height; }

void WindowGLFW::setTitle(const std::string& title) {
  if (m_window != nullptr) {
    glfwSetWindowTitle(m_window, title.c_str());
  }
}

void WindowGLFW::setResizable(bool resizable) {
  if (m_window != nullptr) {
    glfwSetWindowAttrib(m_window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
  }
}

void WindowGLFW::setFullscreen(bool fullscreen) {
  if (m_window == nullptr) {
    return;
  }

  if (fullscreen && !m_fullscreen) {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    m_width = mode->width;
    m_height = mode->height;
    m_fullscreen = true;
  } else if (!fullscreen && m_fullscreen) {
    glfwSetWindowMonitor(m_window, nullptr, 100, 100, static_cast<int>(m_width), static_cast<int>(m_height), 0);
    m_fullscreen = false;
  }
}

GLFWwindow* WindowGLFW::getNativeWindow() const { return m_window; }

void WindowGLFW::cleanup() {
  if (m_window != nullptr) {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
  }
  if (m_initialized) {
    glfwTerminate();
    m_initialized = false;
  }
}

}  // namespace gfx
