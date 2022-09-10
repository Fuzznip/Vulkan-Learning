#pragma once

#include "core/window/window.hpp"

class VulkanSwapchain
{
public:
  void init(VkPhysicalDevice gpu, VkDevice device, VkSurfaceKHR surface, const Window& window);

  [[nodiscard]]
  VkFormat get_image_format() const { return swapchainImageFormat; }

  [[nodiscard]]
  uint32_t get_image_count() const { return swapchainImages.size(); }

  [[nodiscard]]
  const VkImageView* get_image_view(uint32_t index) const { return &swapchainImageViews[index]; }

  [[nodiscard]]
  const VkSwapchainKHR& get_swap_chain() const { return swapchain; }

  [[nodiscard]]
  const VkExtent2D& get_extents() const { return swapchainExtents; }

  void cleanup(VkDevice device);

private:
  VkSwapchainKHR swapchain;
  VkFormat swapchainImageFormat;
  VkExtent2D swapchainExtents;

  std::vector<VkImage> swapchainImages;
  std::vector<VkImageView> swapchainImageViews;
};
