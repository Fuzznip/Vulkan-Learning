#include <pch.hpp>
#include "core/renderer/vk_pipeline.hpp"

#include "core/renderer/vk_initializers.hpp"

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
{
  // make viewport state from our stored single viewport and single scissor.
  VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .pNext = nullptr,

    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor
  };

  // the blending is just "no blend", but we do write to the color attachment
  // 
  // VkPipelineColorBlendStateCreateInfo contains the information about the attachments and how they are used. 
  // This one has to match the fragment shader outputs.
  VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .pNext = nullptr,

    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment
  };

  // Build the actual pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo{
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = nullptr,

    .stageCount = (uint32_t)shaderStages.size(),
    .pStages = shaderStages.data(),
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState = &viewportStateCreateInfo,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pDepthStencilState = &depthStencil,
    .pColorBlendState = &colorBlendStateCreateInfo,
    .layout = pipelineLayout,
    .renderPass = pass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE
  };

  VkPipeline pipeline;
  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
  {
    std::cout << "Failed to create graphics pipeline!\n";
    return nullptr;
  }

  return pipeline;
}
