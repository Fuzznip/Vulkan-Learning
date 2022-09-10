#include "vk_initializers.hpp"

VkCommandPoolCreateInfo vkinit::command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
  return VkCommandPoolCreateInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = nullptr,

    // Signify the pool as one that can submit graphics commands
    // and allow for resetting of individual command buffers
    .flags = flags,
    .queueFamilyIndex = queueFamilyIndex
  };
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(VkCommandPool pool, uint32_t count, VkCommandBufferLevel level)
{
  return VkCommandBufferAllocateInfo{
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = nullptr,

    // Command buffers can be Primary of Secondary level. Primary level are the ones 
    // that are sent into a VkQueue, and do all of the work. Secondary level are ones 
    // that can act as “subcommands” to a primary buffer and are used in advanced
    // multithreaded scenarios
    .commandPool = pool,
    .level = level,
    .commandBufferCount = count
  };
}
