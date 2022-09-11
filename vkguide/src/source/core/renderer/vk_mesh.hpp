#pragma once

#include <core/renderer/vk_types.hpp>

struct VertexInputDescription
{
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;

  VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex
{
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;

  static VertexInputDescription get_vertex_description();
};

struct Mesh
{
	std::vector<Vertex> vertices;

	AllocatedBuffer vertexBuffer;

  void upload(VmaAllocator allocator);
};

Mesh load_from_obj(const std::string& filepath, const std::string& mtlDir = "");