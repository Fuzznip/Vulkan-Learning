#pragma once

#include "core/renderer/vk_types.hpp"

namespace vkinit
{
  [[nodiscard]]
  VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

  [[nodiscard]]
  VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  [[nodiscard]]
  bool load_shader_module(const std::string& filePath, VkDevice device, VkShaderModule& out);

  [[nodiscard]]
  VkPipelineShaderStageCreateInfo shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
  
  [[nodiscard]]
  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
  
  [[nodiscard]]
  VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);

  [[nodiscard]]
  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode);

  [[nodiscard]]
  VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();

  [[nodiscard]]
  VkPipelineColorBlendAttachmentState color_blend_attachment_state();

  [[nodiscard]]
  VkPipelineLayoutCreateInfo pipeline_layout_create_info();

  [[nodiscard]]
  VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

  [[nodiscard]]
  VkImageViewCreateInfo image_view_create_info(VkFormat format, VkImage image, VkImageAspectFlags flags);

  [[nodiscard]]
  VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);

  [[nodiscard]]
  VkDescriptorSetLayoutBinding descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);

  [[nodiscard]]
  VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo , uint32_t binding);
}