#include <pch.hpp>
#include "vk_textures.hpp"

#include "core/renderer/vk_initializers.hpp"

namespace vkutil
{
  bool load_image(VulkanRenderer& renderer, const std::string& filePath, AllocatedImage& outImage)
  {
    std::cout << fmt::format("Loading texture: {}\n", filePath);
    const auto t1 = std::chrono::high_resolution_clock::now();

    int width, height, channels;

    stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels)
    {
      std::cout << fmt::format("Failed to load texture file: {}\n", filePath);
      return false;
    }

    void* pixelPtr = pixels;
    VkDeviceSize imageSize = 4ull * width * height; // 4 = rgba

    // The format R8G8B8A8 matches exactly with the pixels loaded from stb_image lib
    VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

    AllocatedBuffer staging = renderer.create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(renderer.allocator, staging.alloc, &data);
    memcpy(data, pixelPtr, imageSize);
    vmaUnmapMemory(renderer.allocator, staging.alloc);

    // Data now in staging, don't need CPU image data anymore
    stbi_image_free(pixels);

    VkExtent3D imageExtent{
      .width = (uint32_t)width,
      .height = (uint32_t)height,
      .depth = 1
    };

    auto imgInfo = vkinit::image_create_info(imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

    AllocatedImage image;

    VmaAllocationCreateInfo imgAllocInfo{
      .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO,
    };

    vmaCreateImage(renderer.allocator, &imgInfo, &imgAllocInfo, &image.image, &image.alloc, nullptr);

    renderer.immediate_submit([=](VkCommandBuffer cmd){
      VkImageSubresourceRange range{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      };

      VkImageMemoryBarrier copyBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,

        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,

        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = image.image,
        .subresourceRange = range,
      };

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &copyBarrier);

      // Now that our barrier that sets the layout of the image correctly, let's now receive the pixel data from the buffer
      VkBufferImageCopy copyRegion{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,

        .imageSubresource{
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
        .imageExtent = imageExtent
      };

      vkCmdCopyBufferToImage(cmd, staging.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

      // Now that the image data has been transferred, set the image layout one more time to make it shader readable

      VkImageMemoryBarrier imageBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,

        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,

        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .image = image.image,
        .subresourceRange = range,
      };

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);
    });

    vmaDestroyBuffer(renderer.allocator, staging.buffer, staging.alloc);

    outImage = image;
    
    const auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << fmt::format("Successfully loaded texture [{}] in {:.4} seconds\n", filePath, std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() / 1000000000.0);

    return true;
  }
}
