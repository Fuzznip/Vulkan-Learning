#pragma once

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};
