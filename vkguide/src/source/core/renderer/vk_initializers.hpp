#pragma once

#include "vk_types.hpp"

#define VK_CHECK(x) do { VkResult err = x; if(err) throw std::runtime_error(fmt::format("Detected Vulkan error: {}", err)); } while(0)

namespace vkinit
{
  [[nodiscard]]
  VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

  [[nodiscard]]
  VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}