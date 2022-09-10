#pragma once

#include "core/window/window.hpp"
#include "core/renderer/vk_swapchain.hpp"

class VulkanRenderer
{
public:
  void init(const std::string& appName, const Window& window, bool enableValidationLayers);

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

  VkDebugUtilsMessengerEXT debugMessenger; // Vulkan debug output handle
};
