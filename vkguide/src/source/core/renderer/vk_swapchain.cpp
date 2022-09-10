#include <pch.hpp>

#include "vk_swapchain.hpp"

void VulkanSwapchain::init(VkPhysicalDevice gpu, VkDevice device, VkSurfaceKHR surface, const Window& window)
{
  vkb::Swapchain vkbSwapchain = vkb::SwapchainBuilder{ gpu, device, surface }
    .use_default_format_selection()
    .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
    .set_desired_extent(window.get_width(), window.get_height())
    .build()
    .value();

  swapchain = vkbSwapchain.swapchain;
  swapchainImages = vkbSwapchain.get_images().value();
  swapchainImageViews = vkbSwapchain.get_image_views().value();

  swapchainImageFormat = vkbSwapchain.image_format;
}

void VulkanSwapchain::cleanup(VkDevice device)
{
  vkDestroySwapchainKHR(device, swapchain, nullptr);

  for(const auto& imageView : swapchainImageViews)
    vkDestroyImageView(device, imageView, nullptr);
}
