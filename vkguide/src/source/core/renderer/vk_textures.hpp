#include "core/renderer/vk_renderer.hpp"
#include "core/renderer/vk_types.hpp"

namespace vkutil
{
  [[nodiscard]]
  bool load_image(VulkanRenderer& renderer, const std::string& filePath, AllocatedImage& outImage);
}
