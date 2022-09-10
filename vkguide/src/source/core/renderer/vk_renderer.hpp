#pragma once

#include "core/window/window.hpp"
#include "core/renderer/vk_mesh.hpp"
#include "core/renderer/vk_pipeline.hpp"
#include "core/renderer/vk_swapchain.hpp"

// Camera data?
struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

class VulkanRenderer
{
public:
  void init(const std::string& appName, const Window& window, bool enableValidationLayers);

  void draw(const Window& window);

  void swap_pipeline();

  void cleanup();

private:
  VmaAllocator allocator;

  VkInstance instance;
  VkPhysicalDevice gpu;
  VkDevice device;
  VkSurfaceKHR surface;

  VulkanSwapchain swapchain;

  // Maybe this union should be its own class? Unsure
  VkQueue graphicsQueue;
  uint32_t graphicsQueueFamily;

  // This too? Unsure
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;

  // Should this couple with swapchain? Need the imageviews to sync with framebuffers count
  VkRenderPass renderPass;
  std::vector<VkFramebuffer> framebuffers;
  
  VkPipeline trianglePipeline;
	VkPipeline redTrianglePipeline;
  VkPipeline meshPipeline;
  Mesh triangleMesh;
  Mesh monkeyMesh;
  VkPipelineLayout pipelineLayout;
	VkPipelineLayout meshPipelineLayout;

  VkSemaphore presentSemaphore, renderSemaphore;
  VkFence renderFence;

  VkDebugUtilsMessengerEXT debugMessenger; // Vulkan debug output handle

  uint64_t frameNumber = 0;

  int shader = 0;
};
