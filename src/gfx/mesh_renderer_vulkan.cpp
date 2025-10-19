#include <algorithm>
#include <print>

#include <gfx/mesh_renderer_vulkan.hpp>

namespace gfx {

MeshRendererVulkan::MeshRendererVulkan()
    : m_meshPipeline(VK_NULL_HANDLE),
      m_pbrPipeline(VK_NULL_HANDLE),
      m_edgePipeline(VK_NULL_HANDLE),
      m_pointPipeline(VK_NULL_HANDLE),
      m_meshPipelineLayout(VK_NULL_HANDLE),
      m_pbrPipelineLayout(VK_NULL_HANDLE),
      m_edgePipelineLayout(VK_NULL_HANDLE),
      m_pointPipelineLayout(VK_NULL_HANDLE),
      m_meshVertShader(VK_NULL_HANDLE),
      m_meshFragShader(VK_NULL_HANDLE),
      m_pbrVertShader(VK_NULL_HANDLE),
      m_pbrFragShader(VK_NULL_HANDLE),
      m_edgeVertShader(VK_NULL_HANDLE),
      m_edgeFragShader(VK_NULL_HANDLE),
      m_pointVertShader(VK_NULL_HANDLE),
      m_pointFragShader(VK_NULL_HANDLE),
      m_meshDescriptorSetLayout(VK_NULL_HANDLE),
      m_pbrDescriptorSetLayout(VK_NULL_HANDLE) {}

MeshRendererVulkan::~MeshRendererVulkan() { cleanupPipelines(); }

void MeshRendererVulkan::cleanupPipelines() {
  VkDevice device = getDevice();

  if (m_meshPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_meshPipeline, nullptr);
  if (m_pbrPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_pbrPipeline, nullptr);
  if (m_edgePipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_edgePipeline, nullptr);
  if (m_pointPipeline != VK_NULL_HANDLE) vkDestroyPipeline(device, m_pointPipeline, nullptr);

  if (m_meshPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_meshPipelineLayout, nullptr);
  if (m_pbrPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_pbrPipelineLayout, nullptr);
  if (m_edgePipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_edgePipelineLayout, nullptr);
  if (m_pointPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_pointPipelineLayout, nullptr);

  if (m_meshVertShader != VK_NULL_HANDLE) vkDestroyShaderModule(device, m_meshVertShader, nullptr);
  if (m_meshFragShader != VK_NULL_HANDLE) vkDestroyShaderModule(device, m_meshFragShader, nullptr);
  if (m_pbrVertShader != VK_NULL_HANDLE) vkDestroyShaderModule(device, m_pbrVertShader, nullptr);
  if (m_pbrFragShader != VK_NULL_HANDLE) vkDestroyShaderModule(device, m_pbrFragShader, nullptr);
  if (m_edgeVertShader != VK_NULL_HANDLE) vkDestroyShaderModule(device, m_edgeVertShader, nullptr);
  if (m_edgeFragShader != VK_NULL_HANDLE) vkDestroyShaderModule(device, m_edgeFragShader, nullptr);
  if (m_pointVertShader != VK_NULL_HANDLE) vkDestroyShaderModule(device, m_pointVertShader, nullptr);
  if (m_pointFragShader != VK_NULL_HANDLE) vkDestroyShaderModule(device, m_pointFragShader, nullptr);

  if (m_meshDescriptorSetLayout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(device, m_meshDescriptorSetLayout, nullptr);
  if (m_pbrDescriptorSetLayout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(device, m_pbrDescriptorSetLayout, nullptr);
}

// Stub implementations for now - will implement these properly
TextureGPU MeshRendererVulkan::uploadTexture(const Texture& texture) {
  TextureGPU gpu;
  if (!texture.isValid()) return gpu;

  std::println("Vulkan uploadTexture: {} x {} x {}", texture.width, texture.height, texture.channels);
  // TODO: Implement actual texture upload
  return gpu;
}

void MeshRendererVulkan::freeTexture(TextureGPU& textureGPU) { textureGPU.id = 0; }

MeshGPU MeshRendererVulkan::uploadMesh(const Mesh3D& mesh) {
  MeshGPU meshGPU;
  if (mesh.vertices.empty()) return meshGPU;

  std::println("Vulkan uploadMesh: {} vertices, {} faces", mesh.vertices.size(), mesh.faces.size() / 3);
  // TODO: Implement actual mesh upload
  return meshGPU;
}

void MeshRendererVulkan::drawMesh(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint, bool wireframe) {
  if (!meshGPU.isValid()) return;
  // TODO: Implement actual mesh drawing
  (void)mvp;
  (void)tint;
  (void)wireframe;
}

void MeshRendererVulkan::drawMeshPBR(const MeshGPU& meshGPU, const glm::mat4& model, const glm::mat4& view,
                                     const glm::mat4& projection, const PBRMaterial& material,
                                     const glm::vec3& cameraPos, const glm::vec3& lightPos, const Color& lightColor) {
  if (!meshGPU.isValid()) return;
  // TODO: Implement actual PBR drawing
  (void)model;
  (void)view;
  (void)projection;
  (void)material;
  (void)cameraPos;
  (void)lightPos;
  (void)lightColor;
}

void MeshRendererVulkan::drawMeshEdges(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint,
                                       float lineWidth) {
  if (!meshGPU.hasEdges()) return;
  // TODO: Implement edge drawing
  (void)mvp;
  (void)tint;
  (void)lineWidth;
}

void MeshRendererVulkan::drawMeshPoints(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint,
                                        float pointSize) {
  if (!meshGPU.hasPoints()) return;
  // TODO: Implement point drawing
  (void)mvp;
  (void)tint;
  (void)pointSize;
}

void MeshRendererVulkan::freeMesh(MeshGPU& meshGPU) {
  meshGPU.vao = 0;
  meshGPU.vbo = 0;
  meshGPU.vertexCount = 0;
}

bool MeshRendererVulkan::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                                      VulkanBuffer& buffer) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = memoryUsage;

  VkResult result =
      vmaCreateBuffer(getAllocator(), &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr);
  if (result == VK_SUCCESS) {
    buffer.size = size;
    return true;
  }

  return false;
}

void MeshRendererVulkan::destroyBuffer(VulkanBuffer& buffer) {
  if (buffer.buffer != VK_NULL_HANDLE) {
    vmaDestroyBuffer(getAllocator(), buffer.buffer, buffer.allocation);
    buffer.buffer = VK_NULL_HANDLE;
    buffer.allocation = VK_NULL_HANDLE;
    buffer.size = 0;
  }
}

bool MeshRendererVulkan::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                     VkImageUsageFlags usage, VmaMemoryUsage memoryUsage, VulkanImage& image) {
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = memoryUsage;

  VkResult result = vmaCreateImage(getAllocator(), &imageInfo, &allocInfo, &image.image, &image.allocation, nullptr);
  return result == VK_SUCCESS;
}

void MeshRendererVulkan::destroyImage(VulkanImage& image) {
  VkDevice device = getDevice();

  if (image.sampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, image.sampler, nullptr);
    image.sampler = VK_NULL_HANDLE;
  }

  if (image.view != VK_NULL_HANDLE) {
    vkDestroyImageView(device, image.view, nullptr);
    image.view = VK_NULL_HANDLE;
  }

  if (image.image != VK_NULL_HANDLE) {
    vmaDestroyImage(getAllocator(), image.image, image.allocation);
    image.image = VK_NULL_HANDLE;
    image.allocation = VK_NULL_HANDLE;
  }
}

VkShaderModule MeshRendererVulkan::createShaderModule(const std::vector<uint32_t>& code) {
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size() * sizeof(uint32_t);
  createInfo.pCode = code.data();

  VkShaderModule shaderModule;
  VkResult result = vkCreateShaderModule(getDevice(), &createInfo, nullptr, &shaderModule);

  if (result != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return shaderModule;
}

// Pipeline creation stubs - will implement these when adding shaders
bool MeshRendererVulkan::createMeshPipeline() {
  // TODO: Implement
  return true;
}

bool MeshRendererVulkan::createPBRPipeline() {
  // TODO: Implement
  return true;
}

bool MeshRendererVulkan::createEdgePipeline() {
  // TODO: Implement
  return true;
}

bool MeshRendererVulkan::createPointPipeline() {
  // TODO: Implement
  return true;
}

}  // namespace gfx
