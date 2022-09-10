#include <pch.hpp>

#include "core/renderer/vk_renderer.hpp"

#include "core/renderer/vk_initializers.hpp"

void VulkanRenderer::init(const std::string& appName, const Window& window, bool enableValidationLayers)
{
  vkb::Instance bootstrapInstance = vkb::InstanceBuilder{}
    .set_app_name(appName.c_str())
    .request_validation_layers(true) // TODO: Only enable validation layers on debug or specific builds instead of default always
    .require_api_version(1, 3, 0)
    .use_default_debug_messenger() // same here as above ^
    .build() // Can check success value here but if this dont work idk what to tell u brev
    .value();

  instance = bootstrapInstance.instance;
  debugMessenger = bootstrapInstance.debug_messenger;

  surface = window.create_surface(instance);

  vkb::PhysicalDevice selectedGpu = vkb::PhysicalDeviceSelector{ bootstrapInstance }
    .set_minimum_version(1, 3)
    .set_surface(surface)
    .select()
    .value();

  vkb::Device gpuDevice = vkb::DeviceBuilder{ selectedGpu }.build().value();

  gpu = selectedGpu.physical_device;
  std::cout << selectedGpu.name << '\n';
  device = gpuDevice.device;

  swapchain.init(gpu, device, surface, window);

  graphicsQueue = gpuDevice.get_queue(vkb::QueueType::graphics).value();
  graphicsQueueFamily = gpuDevice.get_queue_index(vkb::QueueType::graphics).value();

  // init commands
  {
    auto commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool));

    auto cmdAllocInfo = vkinit::command_buffer_allocate_info(commandPool, 1);

    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer));
  }

  // init render pass
  {
    // Display image attachment
    VkAttachmentDescription colorAttachment{
      .format = swapchain.get_image_format(), // Set format to one needed by swapchain (So we can render it later)
      .samples = VK_SAMPLE_COUNT_1_BIT, // No MSAA

      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // Clear this attachment on load
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Keep the attachment stored when renderpass ends

      // Don't care about stencil
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Don't care about starting layout
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // After render pass, layout should be ready to present to swap chain
    };

    // The reference to the color attachment for our subpass to use
    VkAttachmentReference colorAttachmentRef{
      .attachment = 0, // Index into pAttachments array in parent render pass
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // Set image layout to one optimal for rendering
    };

    // Our single subpass for this render pass
    VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef
    };

    VkRenderPassCreateInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = nullptr,

      .attachmentCount = 1,
      .pAttachments = &colorAttachment,

      .subpassCount = 1,
      .pSubpasses = &subpass
    };

    VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
  }

  // init framebuffers
  {
    VkFramebufferCreateInfo framebufferInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,

      .renderPass = renderPass,
      .attachmentCount = 1,
      .width = window.get_width(),
      .height = window.get_height(),
      .layers = 1
    };

    framebuffers.resize(swapchain.get_image_count());
    for (int i = 0; i < framebuffers.size(); ++i)
    {
      framebufferInfo.pAttachments = swapchain.get_image_view(i);
      VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]));
    }
  }

  // init sync objects
  {
    VkFenceCreateInfo fenceInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,

      .flags = VK_FENCE_CREATE_SIGNALED_BIT // Create the fence already signaled so that first pass will not hang
    };

    VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &renderFence));

    VkSemaphoreCreateInfo semaphoreInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = nullptr,

      .flags = 0
    };

    VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &presentSemaphore));
    VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderSemaphore));
  }
}

void VulkanRenderer::draw(const Window& window)
{
  constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();

  VK_CHECK(vkWaitForFences(device, 1, &renderFence, VK_TRUE, timeout));
	VK_CHECK(vkResetFences(device, 1, &renderFence));

  uint32_t swapchainImageIndex;
  VK_CHECK(vkAcquireNextImageKHR(device, swapchain.get_swap_chain(), timeout, presentSemaphore, nullptr, &swapchainImageIndex));

  // Now that rendering is finished for last frame, we can begin our rendering commands
  VK_CHECK(vkResetCommandBuffer(commandBuffer, 0));

  // Record our draw commands
  {
    VkCommandBufferBeginInfo cmdBegin{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,

      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr,
    };

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBegin));
    {
      VkClearValue clearColor{
        .color = {{ 0.f, 0.f, std::abs(std::sin(frameNumber / 120.f)), 1.f }}
      };

      VkRenderPassBeginInfo renderpassBegin{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,

        .renderPass = renderPass,
        .framebuffer = framebuffers[swapchainImageIndex],
        .renderArea{ 
          .offset{ .x = 0, .y = 0 },
          .extent{ swapchain.get_extents() }
        },

        .clearValueCount = 1,
        .pClearValues = &clearColor
      };

      vkCmdBeginRenderPass(commandBuffer, &renderpassBegin, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdEndRenderPass(commandBuffer);
    }
    VK_CHECK(vkEndCommandBuffer(commandBuffer));
  }

  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = nullptr,

    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &presentSemaphore,
    .pWaitDstStageMask = &waitStage,

    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,

    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &renderSemaphore,
  };

  VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, renderFence));

  VkPresentInfoKHR presentInfo{
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = nullptr,

    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &renderSemaphore,

    .swapchainCount = 1,
    .pSwapchains = &swapchain.get_swap_chain(),

    .pImageIndices = &swapchainImageIndex
  };

  VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

  frameNumber += 1;
}

void VulkanRenderer::cleanup()
{
  constexpr auto timeout = 1000000000; // 1 second (in nanoseconds)

  vkWaitForFences(device, 1, &renderFence, VK_TRUE, 1000000000);
  vkDestroyFence(device, renderFence, nullptr);

  vkDestroySemaphore(device, renderSemaphore, nullptr);
  vkDestroySemaphore(device, presentSemaphore, nullptr);

  vkDestroyCommandPool(device, commandPool, nullptr);

  vkDestroyRenderPass(device, renderPass, nullptr);
  for(const auto& framebuffer : framebuffers)
    vkDestroyFramebuffer(device, framebuffer, nullptr);

  swapchain.cleanup(device);

  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkb::destroy_debug_utils_messenger(instance, debugMessenger);
  vkDestroyInstance(instance, nullptr);
}
