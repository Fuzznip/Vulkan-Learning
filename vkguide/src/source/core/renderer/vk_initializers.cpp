#include "vk_initializers.hpp"

#include "core/filesystem/read_file.hpp"

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

bool vkinit::load_shader_module(const std::string& filePath, VkDevice device, VkShaderModule& out)
{
  auto buf = read_file(filePath);

  if(!buf)
    return false;

  VkShaderModuleCreateInfo createInfo{
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = nullptr,

    .codeSize = buf->size(),
    .pCode = reinterpret_cast<const uint32_t*>(buf->data())
  };

  return vkCreateShaderModule(device, &createInfo, nullptr, &out) == VK_SUCCESS;
}

VkPipelineShaderStageCreateInfo vkinit::shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule)
{
  return VkPipelineShaderStageCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = nullptr,

    .stage = stage,
    .module = shaderModule,
    .pName = "main"
  };
}

VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info()
{
  return VkPipelineVertexInputStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .pNext = nullptr
  };
}

VkPipelineInputAssemblyStateCreateInfo vkinit::input_assembly_create_info(VkPrimitiveTopology topology)
{
  return VkPipelineInputAssemblyStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .pNext = nullptr,

    .topology = topology,
    .primitiveRestartEnable = VK_FALSE
  };
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterization_state_create_info(VkPolygonMode polygonMode)
{
  return VkPipelineRasterizationStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .pNext = nullptr,

    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,

    .polygonMode = polygonMode,
    .cullMode = VK_CULL_MODE_NONE,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,

    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.f,
    .depthBiasClamp = 0.f,
    .depthBiasSlopeFactor = 0.f,

    .lineWidth = 1.f,
  };
}

VkPipelineMultisampleStateCreateInfo vkinit::multisampling_state_create_info()
{
  return VkPipelineMultisampleStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .pNext = nullptr,

    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT, // MSAA not enabled
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.f,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };
}

VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state()
{
  return VkPipelineColorBlendAttachmentState{
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };
}

VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info()
{
  return VkPipelineLayoutCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = nullptr,

    .flags = 0,
    .setLayoutCount = 0,
    .pSetLayouts = nullptr,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = nullptr
  };
}

VkImageCreateInfo vkinit::image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
{
  return VkImageCreateInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = nullptr,

    .imageType = VK_IMAGE_TYPE_2D,

    .format = format,
    .extent = extent,

    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = usageFlags
  };
}

VkImageViewCreateInfo vkinit::image_view_create_info(VkFormat format, VkImage image, VkImageAspectFlags flags)
{
  return VkImageViewCreateInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = nullptr,

    .image = image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = format,
    .subresourceRange{
      .aspectMask = flags,

      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
  };
}

VkPipelineDepthStencilStateCreateInfo vkinit::depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp)
{
  return VkPipelineDepthStencilStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext = nullptr,

    .depthTestEnable = bDepthTest,
    .depthWriteEnable = bDepthWrite,
    .depthCompareOp = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .minDepthBounds = 0.f,
    .maxDepthBounds = 1.f,
  };
}

VkDescriptorSetLayoutBinding vkinit::descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding)
{
	return VkDescriptorSetLayoutBinding{
    .binding = binding,
    .descriptorType = type,
    .descriptorCount = 1,
    .stageFlags = stageFlags,

    .pImmutableSamplers = nullptr,
  };
}

VkWriteDescriptorSet vkinit::write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding)
{
  return VkWriteDescriptorSet{
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .pNext = nullptr,

    .dstSet = dstSet,
    .dstBinding = binding,
    .descriptorCount = 1,
    .descriptorType = type,
    .pBufferInfo = bufferInfo,
  };
}
