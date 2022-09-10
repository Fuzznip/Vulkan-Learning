#pragma once

#define VK_CHECK(x) do { VkResult err = x; if(err) throw std::runtime_error(fmt::format("Detected Vulkan error: {}", err)); } while(0)

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};
