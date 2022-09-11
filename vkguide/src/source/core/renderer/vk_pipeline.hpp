#pragma once

struct PipelineBuilder
{
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineLayout pipelineLayout;

	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};
