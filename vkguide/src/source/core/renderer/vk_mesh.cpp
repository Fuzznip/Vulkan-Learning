#include <pch.hpp>
#include "vk_mesh.hpp"

VertexInputDescription Vertex::get_vertex_description()
{
  VertexInputDescription vid;

  VkVertexInputAttributeDescription;
  VkVertexInputBindingDescription;
  VkPipelineVertexInputStateCreateFlags;

  VkVertexInputBindingDescription vertBinding{
    .binding = 0,
    .stride = sizeof Vertex,
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
  };

  vid.bindings = { vertBinding };

  VkVertexInputAttributeDescription posAttrib{
    .location = 0,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(Vertex, position)
  };

  VkVertexInputAttributeDescription normalAttrib{
    .location = 1,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(Vertex, normal)
  };

  VkVertexInputAttributeDescription colorAttrib{
    .location = 2,
    .binding = 0,
    .format = VK_FORMAT_R32G32B32_SFLOAT,
    .offset = offsetof(Vertex, color)
  };

  vid.attributes = {
    posAttrib,
    normalAttrib,
    colorAttrib
  };

  return vid;
}
