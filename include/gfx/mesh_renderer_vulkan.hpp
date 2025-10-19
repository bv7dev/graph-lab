#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <gfx/renderer_vulkan.hpp>
#include <util/types.hpp>
#include <util/glm.hpp>

namespace gfx {

using namespace util;

// Vulkan buffer helper
struct VulkanBuffer {
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
  VkDeviceSize size = 0;
  
  [[nodiscard]] bool isValid() const { return buffer != VK_NULL_HANDLE; }
};

// Vulkan image helper
struct VulkanImage {
  VkImage image = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VmaAllocation allocation = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  
  [[nodiscard]] bool isValid() const { return image != VK_NULL_HANDLE; }
};

// PBR material for Vulkan
struct PBRMaterialVulkan {
  Color baseColor{1.0f, 1.0f, 1.0f, 1.0f};
  float metallic = 0.0f;
  float roughness = 0.5f;
  
  // Texture handles
  VulkanImage baseColorTexture;
  VulkanImage metallicRoughnessTexture;
  VulkanImage normalTexture;
};

class MeshRendererVulkan : public RendererVulkan {
 public:
  MeshRendererVulkan();
  ~MeshRendererVulkan();

  MeshRendererVulkan(const MeshRendererVulkan&) = delete;
  MeshRendererVulkan(MeshRendererVulkan&&) = delete;
  MeshRendererVulkan& operator=(const MeshRendererVulkan&) = delete;
  MeshRendererVulkan& operator=(MeshRendererVulkan&&) = delete;

  // Texture management
  TextureGPU uploadTexture(const Texture& texture);
  static void freeTexture(TextureGPU& textureGPU);

  // Mesh rendering API
  MeshGPU uploadMesh(const Mesh3D& mesh);
  void drawMesh(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f),
                bool wireframe = false);
  void drawMeshPBR(const MeshGPU& meshGPU, const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection,
                   const PBRMaterial& material, const glm::vec3& cameraPos, const glm::vec3& lightPos,
                   const Color& lightColor = Color(1.0f, 1.0f, 1.0f, 1.0f));
  void drawMeshEdges(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f),
                     float lineWidth = 1.0f);
  void drawMeshPoints(const MeshGPU& meshGPU, const glm::mat4& mvp, const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f),
                      float pointSize = 1.0f);
  static void freeMesh(MeshGPU& meshGPU);

 private:
  // Pipeline and shaders
  VkPipeline m_meshPipeline;
  VkPipeline m_pbrPipeline;
  VkPipeline m_edgePipeline;
  VkPipeline m_pointPipeline;
  
  VkPipelineLayout m_meshPipelineLayout;
  VkPipelineLayout m_pbrPipelineLayout;
  VkPipelineLayout m_edgePipelineLayout;
  VkPipelineLayout m_pointPipelineLayout;
  
  VkShaderModule m_meshVertShader;
  VkShaderModule m_meshFragShader;
  VkShaderModule m_pbrVertShader;
  VkShaderModule m_pbrFragShader;
  VkShaderModule m_edgeVertShader;
  VkShaderModule m_edgeFragShader;
  VkShaderModule m_pointVertShader;
  VkShaderModule m_pointFragShader;
  
  // Descriptor sets
  VkDescriptorSetLayout m_meshDescriptorSetLayout;
  VkDescriptorSetLayout m_pbrDescriptorSetLayout;

  void cleanupPipelines();
  bool createMeshPipeline();
  bool createPBRPipeline();
  bool createEdgePipeline();
  bool createPointPipeline();
  
  VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
  bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VulkanBuffer& buffer);
  void destroyBuffer(VulkanBuffer& buffer);
  bool createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                   VmaMemoryUsage memoryUsage, VulkanImage& image);
  void destroyImage(VulkanImage& image);
};

}  // namespace gfx
