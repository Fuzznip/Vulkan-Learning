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
}

void VulkanRenderer::cleanup()
{
  vkDestroyCommandPool(device, commandPool, nullptr);

  swapchain.cleanup(device);

  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkb::destroy_debug_utils_messenger(instance, debugMessenger);
  vkDestroyInstance(instance, nullptr);
}
