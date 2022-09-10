#include "C:/Users/kento/source/repos/Fuzznip/Vulkan-Learning/vkguide/build/CMakeFiles/VkGuide.dir/Debug/cmake_pch.hxx"
#include "read_file.hpp"

std::optional<std::vector<unsigned char>> read_file(const std::string& filePath)
{
  std::ifstream file(filePath, std::ios::ate | std::ios::binary);

  if(!file.is_open())
    return std::nullopt;

  size_t fileSize = file.tellg();
  std::vector<unsigned char> buffer(fileSize);
  file.seekg(0);
  file.read((char*)buffer.data(), fileSize);
  file.close();

  return buffer;
}
