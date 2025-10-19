#include <algorithm>
#include <array>
#include <cstring>
#include <print>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <gfx/renderer_vulkan.hpp>
#include <gfx/window_glfw.hpp>

namespace gfx {

RendererVulkan::RendererVulkan()
    : m_surface(VK_NULL_HANDLE),
      m_graphicsQueue(VK_NULL_HANDLE),
      m_presentQueue(VK_NULL_HANDLE),
      m_commandPool(VK_NULL_HANDLE),
      m_currentFrame(0),
      m_imageIndex(0),
      m_renderPass(VK_NULL_HANDLE),
      m_depthImage(VK_NULL_HANDLE),
      m_depthImageView(VK_NULL_HANDLE),
      m_depthImageAllocation(VK_NULL_HANDLE),
      m_depthFormat(VK_FORMAT_UNDEFINED),
      m_allocator(VK_NULL_HANDLE),
      m_descriptorPool(VK_NULL_HANDLE),
      m_initialized(false),
      m_currentColor(1.0f, 1.0f, 1.0f, 1.0f),
      m_blendingEnabled(false),
      m_clearColor(0.0f, 0.0f, 0.0f, 1.0f),
      m_framebufferResized(false) {
  m_swapchainExtent = {0, 0};
  m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
}

RendererVulkan::~RendererVulkan() { cleanup(); }

bool RendererVulkan::initialize(const WindowGLFW& window, uint32_t width, uint32_t height) {
  if (m_initialized) {
    return false;
  }

  std::println("Vulkan: Starting initialization...");
  GLFWwindow* glfwWindow = window.getNativeWindow();

  // Create Vulkan instance using vk-bootstrap
  std::println("Vulkan: Creating instance...");
  vkb::InstanceBuilder instanceBuilder;
  auto instRet = instanceBuilder.set_app_name("Graph Lab")
                     .request_validation_layers(false)  // Disable validation for WSL2 compatibility
                     .require_api_version(1, 0, 0)      // Lower requirement for llvmpipe
                     .build();

  if (!instRet) {
    std::println(stderr, "Failed to create Vulkan instance: {}", instRet.error().message());
    return false;
  }

  m_vkbInstance = instRet.value();

  // Create surface
  VkResult result = glfwCreateWindowSurface(m_vkbInstance.instance, glfwWindow, nullptr, &m_surface);
  if (result != VK_SUCCESS) {
    std::println(stderr, "Failed to create window surface");
    return false;
  }

  // Select physical device and create logical device (more lenient for WSL2/llvmpipe)
  vkb::PhysicalDeviceSelector selector{m_vkbInstance};
  auto physDevRet = selector.set_surface(m_surface)
                        .set_minimum_version(1, 0)        // Accept Vulkan 1.0 for llvmpipe
                        .allow_any_gpu_device_type(true)  // Accept CPU devices like llvmpipe
                        .select();

  if (!physDevRet) {
    std::println(stderr, "Failed to select physical device: {}", physDevRet.error().message());
    return false;
  }

  vkb::DeviceBuilder deviceBuilder{physDevRet.value()};
  auto devRet = deviceBuilder.build();

  if (!devRet) {
    std::println(stderr, "Failed to create logical device: {}", devRet.error().message());
    return false;
  }

  m_device = devRet.value();

  // Get queues
  auto graphicsQueueRet = m_device.get_queue(vkb::QueueType::graphics);
  if (!graphicsQueueRet) {
    std::println(stderr, "Failed to get graphics queue");
    return false;
  }
  m_graphicsQueue = graphicsQueueRet.value();

  auto presentQueueRet = m_device.get_queue(vkb::QueueType::present);
  if (!presentQueueRet) {
    std::println(stderr, "Failed to get present queue");
    return false;
  }
  m_presentQueue = presentQueueRet.value();

  // Initialize VMA
  if (!initializeAllocator()) {
    return false;
  }

  // Create swapchain
  if (!createSwapchain(width, height)) {
    return false;
  }

  if (!createImageViews()) {
    return false;
  }

  if (!createRenderPass()) {
    return false;
  }

  if (!createDepthResources()) {
    return false;
  }

  if (!createFramebuffers()) {
    return false;
  }

  if (!createCommandPool()) {
    return false;
  }

  if (!createCommandBuffers()) {
    return false;
  }

  if (!createSyncObjects()) {
    return false;
  }

  if (!createDescriptorPool()) {
    return false;
  }

  // Initialize ImGui for Vulkan
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

  ImGui_ImplVulkan_InitInfo initInfo = {};
  initInfo.Instance = m_vkbInstance.instance;
  initInfo.PhysicalDevice = m_device.physical_device.physical_device;
  initInfo.Device = m_device.device;
  initInfo.QueueFamily = m_device.get_queue_index(vkb::QueueType::graphics).value();
  initInfo.Queue = m_graphicsQueue;
  initInfo.PipelineCache = VK_NULL_HANDLE;
  initInfo.DescriptorPool = m_descriptorPool;
  initInfo.MinImageCount = 2;
  initInfo.ImageCount = static_cast<uint32_t>(m_swapchainImages.size());
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  initInfo.Allocator = nullptr;
  initInfo.CheckVkResultFn = nullptr;
  initInfo.RenderPass = m_renderPass;

  ImGui_ImplVulkan_Init(&initInfo);

  // Upload ImGui fonts
  VkCommandBuffer commandBuffer = m_commandBuffers[0];
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  ImGui_ImplVulkan_CreateFontsTexture();
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(m_graphicsQueue);

  ImGui_ImplVulkan_DestroyFontsTexture();

  m_initialized = true;
  std::println("Vulkan renderer initialized successfully");
  return true;
}

bool RendererVulkan::initializeAllocator() {
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
  allocatorInfo.physicalDevice = m_device.physical_device.physical_device;
  allocatorInfo.device = m_device.device;
  allocatorInfo.instance = m_vkbInstance.instance;

  VkResult result = vmaCreateAllocator(&allocatorInfo, &m_allocator);
  return result == VK_SUCCESS;
}

bool RendererVulkan::createSwapchain(uint32_t width, uint32_t height) {
  vkb::SwapchainBuilder swapchainBuilder{m_device};

  auto swapRet = swapchainBuilder.set_old_swapchain(m_swapchain)
                     .set_desired_extent(width, height)
                     .set_desired_format({VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                     .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)  // V-Sync
                     .build();

  if (!swapRet) {
    std::println(stderr, "Failed to create swapchain: {}", swapRet.error().message());
    return false;
  }

  vkb::destroy_swapchain(m_swapchain);
  m_swapchain = swapRet.value();

  m_swapchainImages = m_swapchain.get_images().value();
  m_swapchainImageViews = m_swapchain.get_image_views().value();
  m_swapchainImageFormat = m_swapchain.image_format;
  m_swapchainExtent = m_swapchain.extent;

  return true;
}

bool RendererVulkan::createImageViews() {
  // Already created by vk-bootstrap
  return true;
}

bool RendererVulkan::createRenderPass() {
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = m_swapchainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format = findDepthFormat();
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  VkResult result = vkCreateRenderPass(m_device.device, &renderPassInfo, nullptr, &m_renderPass);
  return result == VK_SUCCESS;
}

bool RendererVulkan::createFramebuffers() {
  m_framebuffers.resize(m_swapchainImageViews.size());

  for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
    std::array<VkImageView, 2> attachments = {m_swapchainImageViews[i], m_depthImageView};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = m_swapchainExtent.width;
    framebufferInfo.height = m_swapchainExtent.height;
    framebufferInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(m_device.device, &framebufferInfo, nullptr, &m_framebuffers[i]);
    if (result != VK_SUCCESS) {
      return false;
    }
  }

  return true;
}

bool RendererVulkan::createCommandPool() {
  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = m_device.get_queue_index(vkb::QueueType::graphics).value();
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkResult result = vkCreateCommandPool(m_device.device, &poolInfo, nullptr, &m_commandPool);
  return result == VK_SUCCESS;
}

bool RendererVulkan::createCommandBuffers() {
  m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = m_commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

  VkResult result = vkAllocateCommandBuffers(m_device.device, &allocInfo, m_commandBuffers.data());
  return result == VK_SUCCESS;
}

bool RendererVulkan::createSyncObjects() {
  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(m_device.device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(m_device.device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(m_device.device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
      return false;
    }
  }

  return true;
}

bool RendererVulkan::createDepthResources() {
  m_depthFormat = findDepthFormat();

  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = m_swapchainExtent.width;
  imageInfo.extent.height = m_swapchainExtent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = m_depthFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VkResult result =
      vmaCreateImage(m_allocator, &imageInfo, &allocInfo, &m_depthImage, &m_depthImageAllocation, nullptr);
  if (result != VK_SUCCESS) {
    return false;
  }

  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = m_depthImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = m_depthFormat;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  result = vkCreateImageView(m_device.device, &viewInfo, nullptr, &m_depthImageView);
  return result == VK_SUCCESS;
}

bool RendererVulkan::createDescriptorPool() {
  VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets = 1000 * 11;
  poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
  poolInfo.pPoolSizes = poolSizes;

  VkResult result = vkCreateDescriptorPool(m_device.device, &poolInfo, nullptr, &m_descriptorPool);
  return result == VK_SUCCESS;
}

VkFormat RendererVulkan::findDepthFormat() {
  return findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                             VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat RendererVulkan::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                                             VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(m_device.physical_device.physical_device, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }

  return VK_FORMAT_UNDEFINED;
}

void RendererVulkan::clear(const Color& color) { m_clearColor = color; }

void RendererVulkan::beginFrame() {
  vkWaitForFences(m_device.device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

  VkResult result = vkAcquireNextImageKHR(m_device.device, m_swapchain.swapchain, UINT64_MAX,
                                          m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    std::println(stderr, "Failed to acquire swapchain image");
    return;
  }

  vkResetFences(m_device.device, 1, &m_inFlightFences[m_currentFrame]);

  VkCommandBuffer commandBuffer = m_commandBuffers[m_currentFrame];
  vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = m_renderPass;
  renderPassInfo.framebuffer = m_framebuffers[m_imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = m_swapchainExtent;

  std::array<VkClearValue, 2> clearValues = {};
  clearValues[0].color = {{m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a}};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void RendererVulkan::endFrame() {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_commandBuffers[m_currentFrame]);

  vkCmdEndRenderPass(m_commandBuffers[m_currentFrame]);
  vkEndCommandBuffer(m_commandBuffers[m_currentFrame]);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

  VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VkResult result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
  if (result != VK_SUCCESS) {
    std::println(stderr, "Failed to submit draw command buffer");
  }

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {m_swapchain.swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &m_imageIndex;

  result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
    m_framebufferResized = false;
    recreateSwapchain();
  } else if (result != VK_SUCCESS) {
    std::println(stderr, "Failed to present swapchain image");
  }

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

float RendererVulkan::getFramerate() {
  ImGuiIO& io = ImGui::GetIO();
  return io.Framerate;
}

VkCommandBuffer RendererVulkan::getCurrentCommandBuffer() const { return m_commandBuffers[m_currentFrame]; }

void RendererVulkan::cleanupSwapchain() {
  if (m_depthImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(m_device.device, m_depthImageView, nullptr);
  }
  if (m_depthImage != VK_NULL_HANDLE) {
    vmaDestroyImage(m_allocator, m_depthImage, m_depthImageAllocation);
  }

  for (auto framebuffer : m_framebuffers) {
    vkDestroyFramebuffer(m_device.device, framebuffer, nullptr);
  }

  vkb::destroy_swapchain(m_swapchain);
}

void RendererVulkan::recreateSwapchain() {
  vkDeviceWaitIdle(m_device.device);

  cleanupSwapchain();

  createSwapchain(m_swapchainExtent.width, m_swapchainExtent.height);
  createDepthResources();
  createFramebuffers();
}

void RendererVulkan::cleanup() {
  if (!m_initialized) {
    return;
  }

  vkDeviceWaitIdle(m_device.device);

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  cleanupSwapchain();

  if (m_descriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(m_device.device, m_descriptorPool, nullptr);
  }

  if (m_renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(m_device.device, m_renderPass, nullptr);
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(m_device.device, m_renderFinishedSemaphores[i], nullptr);
    }
    if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(m_device.device, m_imageAvailableSemaphores[i], nullptr);
    }
    if (m_inFlightFences[i] != VK_NULL_HANDLE) {
      vkDestroyFence(m_device.device, m_inFlightFences[i], nullptr);
    }
  }

  if (m_commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(m_device.device, m_commandPool, nullptr);
  }

  if (m_allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(m_allocator);
  }

  vkb::destroy_device(m_device);

  if (m_surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(m_vkbInstance.instance, m_surface, nullptr);
  }

  vkb::destroy_instance(m_vkbInstance);

  m_initialized = false;
}

// Stub implementations for 2D drawing functions
void RendererVulkan::drawLine(float, float, float, float, const Color&) {}
void RendererVulkan::drawRectangle(float, float, float, float, const Color&, bool) {}
void RendererVulkan::drawCircle(float, float, float, const Color&, bool) {}
void RendererVulkan::drawTriangle(float, float, float, float, float, float, const Color&, bool) {}
void RendererVulkan::drawTriangles(const std::vector<Vertex2D>&) {}
void RendererVulkan::drawLines(const std::vector<Vertex2D>&) {}
void RendererVulkan::setColor(const Color& color) { m_currentColor = color; }
void RendererVulkan::setBlending(bool enabled) { m_blendingEnabled = enabled; }
void RendererVulkan::setViewport(uint32_t width, uint32_t height) {
  m_swapchainExtent.width = width;
  m_swapchainExtent.height = height;
  m_framebufferResized = true;
}

}  // namespace gfx
