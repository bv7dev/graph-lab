#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <util/types.hpp>
#include <util/glm.hpp>

namespace gfx {

using namespace util;

// Forward declaration
class WindowGLFW;

class RendererVulkan {
 public:
  RendererVulkan();
  ~RendererVulkan();

  RendererVulkan(const RendererVulkan&) = delete;
  RendererVulkan(RendererVulkan&&) = delete;
  RendererVulkan& operator=(const RendererVulkan&) = delete;
  RendererVulkan& operator=(RendererVulkan&&) = delete;

  bool initialize(const WindowGLFW& window, uint32_t width, uint32_t height);
  void setViewport(uint32_t width, uint32_t height);
  void clear(const Color& color);
  void beginFrame();
  void endFrame();

  static float getFramerate();

  [[nodiscard]] uint32_t getWidth() const { return m_swapchainExtent.width; }
  [[nodiscard]] uint32_t getHeight() const { return m_swapchainExtent.height; }

  // Basic 2D drawing functions (stubbed for now)
  void drawLine(float x1, float y1, float x2, float y2, const Color& color);
  void drawRectangle(float x, float y, float width, float height, const Color& color, bool filled);
  void drawCircle(float centerX, float centerY, float radius, const Color& color, bool filled);
  void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color& color, bool filled);
  void drawTriangles(const std::vector<Vertex2D>& vertices);
  void drawLines(const std::vector<Vertex2D>& vertices);

  void setColor(const Color& color);
  void setBlending(bool enabled);

  // Vulkan-specific accessors
  [[nodiscard]] VkDevice getDevice() const { return m_device.device; }
  [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return m_device.physical_device.physical_device; }
  [[nodiscard]] VkCommandBuffer getCurrentCommandBuffer() const;
  [[nodiscard]] VkRenderPass getRenderPass() const { return m_renderPass; }
  [[nodiscard]] VmaAllocator getAllocator() const { return m_allocator; }

 protected:
  // vk-bootstrap objects
  vkb::Instance m_vkbInstance;
  vkb::Device m_device;
  vkb::Swapchain m_swapchain;

  // Core Vulkan objects
  VkSurfaceKHR m_surface;
  VkQueue m_graphicsQueue;
  VkQueue m_presentQueue;

  // Swapchain
  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;
  std::vector<VkFramebuffer> m_framebuffers;
  VkExtent2D m_swapchainExtent;
  VkFormat m_swapchainImageFormat;

  // Command buffers
  VkCommandPool m_commandPool;
  std::vector<VkCommandBuffer> m_commandBuffers;

  // Synchronization
  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence> m_inFlightFences;
  uint32_t m_currentFrame;
  uint32_t m_imageIndex;

  // Render pass
  VkRenderPass m_renderPass;

  // Depth buffer
  VkImage m_depthImage;
  VkImageView m_depthImageView;
  VmaAllocation m_depthImageAllocation;
  VkFormat m_depthFormat;

  // Memory allocator
  VmaAllocator m_allocator;

  // Descriptor pool for ImGui
  VkDescriptorPool m_descriptorPool;

  bool m_initialized;
  Color m_currentColor;
  bool m_blendingEnabled;
  Color m_clearColor;
  bool m_framebufferResized;

  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  void cleanup();
  bool createSwapchain(uint32_t width, uint32_t height);
  bool createImageViews();
  bool createRenderPass();
  bool createFramebuffers();
  bool createCommandPool();
  bool createCommandBuffers();
  bool createSyncObjects();
  bool createDepthResources();
  bool createDescriptorPool();
  bool initializeAllocator();

  void cleanupSwapchain();
  void recreateSwapchain();

  VkFormat findDepthFormat();
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features);
};

}  // namespace gfx
