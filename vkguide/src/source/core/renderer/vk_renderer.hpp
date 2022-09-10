#pragma once

#include "core/window/window.hpp"
#include "core/renderer/vk_swapchain.hpp"

class VulkanRenderer
{
public:
  void init(const std::string& appName, const Window& window, bool enableValidationLayers);

  void draw(const Window& window);

  void cleanup();

private:
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

  VkSemaphore presentSemaphore, renderSemaphore;
  VkFence renderFence;

  VkDebugUtilsMessengerEXT debugMessenger; // Vulkan debug output handle

  uint64_t frameNumber = 0;
};
