#pragma once

#include "core/window/window.hpp"

class VulkanSwapchain
{
public:
  void init(VkPhysicalDevice gpu, VkDevice device, VkSurfaceKHR surface, const Window& window);

  void cleanup(VkDevice device);

private:
  VkSwapchainKHR swapchain;
  VkFormat swapchainImageFormat;

  std::vector<VkImage> swapchainImages;
  std::vector<VkImageView> swapchainImageViews;
};
