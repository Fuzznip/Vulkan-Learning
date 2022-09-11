#include <pch.hpp>
#include "core/renderer/vk_renderer.hpp"

#include "core/renderer/vk_initializers.hpp"
#include "core/filesystem/read_file.hpp"

#ifdef NDEBUG
  constexpr bool enableValidationLayers = false;
#else
  constexpr bool enableValidationLayers = true;
#endif

void VulkanRenderer::init(const std::string& appName, const Window& window, bool enableValidationLayers)
{
  vkb::Instance bootstrapInstance = vkb::InstanceBuilder{}
    .set_app_name(appName.c_str())
    .request_validation_layers(enableValidationLayers)
    .require_api_version(1, 3, 0)
#ifndef NDEBUG
    .use_default_debug_messenger()
#endif
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

  graphicsQueue = gpuDevice.get_queue(vkb::QueueType::graphics).value();
  graphicsQueueFamily = gpuDevice.get_queue_index(vkb::QueueType::graphics).value();

  // init vma
  {
    VmaAllocatorCreateInfo allocInfo{
      .physicalDevice = gpu,
      .device = device,
      .instance = instance
    };

    vmaCreateAllocator(&allocInfo, &allocator);
  }

  // init swapchain
  {
    swapchain.init(gpu, device, surface, window);
    
    // Create depth target
    VkExtent3D depthImageExtent{
      .width = window.get_width(),
      .height = window.get_height(),
      .depth = 1
    };

    depthFormat = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo imageInfo = vkinit::image_create_info(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
    VmaAllocationCreateInfo allocInfo{
      .usage = VMA_MEMORY_USAGE_GPU_ONLY,
      .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    VK_CHECK(vmaCreateImage(allocator, &imageInfo, &allocInfo, &depthImage.image, &depthImage.alloc, nullptr));

    VkImageViewCreateInfo imageViewInfo = vkinit::image_view_create_info(depthFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &depthImageView));
  }

  // init commands
  {
    for (int i = 0; i < MaxFramesInFlight; ++i)
    {
      FrameData& frame = frames[i];

      auto commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

      VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frame.cmdPool));

      auto cmdAllocInfo = vkinit::command_buffer_allocate_info(frame.cmdPool, 1);

      VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frame.cmdBuffer));
    }
  }

  // init render pass
  {
    // color attachment
    VkAttachmentDescription colorAttachment{
      .format = swapchain.get_image_format(), // Set format to one needed by swapchain (So we can render it later)
      .samples = VK_SAMPLE_COUNT_1_BIT, // No MSAA

      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // Clear this attachment on load
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Keep the attachment stored when renderpass ends

      // Don't care about stencil
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Don't care about starting layout
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // Final layout should be optimal for presenting to swapchain
    };

    VkAttachmentReference colorAttachmentRef{
      .attachment = 0, // Index into pAttachments array in parent render pass
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // Set image layout to one optimal for rendering
    };

    // depth attachment
    VkAttachmentDescription depthAttachment{
      .format = depthFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT, // No MSAA in depth test
      
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // Clear this attachment on load
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE, // Keep the attachment stored when renderpass ends

      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // Don't care about starting layout
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // Final layout should be optimal for depth
    };

    VkAttachmentReference depthAttachmentRef{
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    // Our single subpass for this render pass
    VkSubpassDescription subpass{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef, // Color attachment
      .pDepthStencilAttachment = &depthAttachmentRef // Depth attachment
    };

    VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
    
    // This dependency set tells Vulkan that the depth attachment in a renderpass cannot be used 
    // before the previous renderpasses has finished using it.
    VkSubpassDependency colorDep{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,

      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,

      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkSubpassDependency depthDep{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,

      .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,

      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    };

    VkSubpassDependency dependencies[2]{ colorDep, depthDep };

    VkRenderPassCreateInfo renderPassInfo{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = nullptr,

      .attachmentCount = 2,
      .pAttachments = attachments,

      .subpassCount = 1,
      .pSubpasses = &subpass,

      .dependencyCount = 2,
      .pDependencies = dependencies
    };

    VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
  }

  // init framebuffers
  {
    VkFramebufferCreateInfo framebufferInfo{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = nullptr,

      .renderPass = renderPass,
      .attachmentCount = 2,
      .width = window.get_width(),
      .height = window.get_height(),
      .layers = 1
    };

    framebuffers.resize(swapchain.get_image_count());
    for (int i = 0; i < framebuffers.size(); ++i)
    {    
      VkImageView attachments[2]{
        *swapchain.get_image_view(i),
        depthImageView,
      };

      framebufferInfo.pAttachments = attachments;
      VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]));
    }
  }

  // init sync objects
  {
    for (int i = 0; i < MaxFramesInFlight; ++i)
    {
      FrameData& frame = frames[i];

      VkFenceCreateInfo fenceInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,

        .flags = VK_FENCE_CREATE_SIGNALED_BIT // Create the fence already signaled so that first pass will not hang
      };

      VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &frame.fence));

      VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,

        .flags = 0
      };

      VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.present));
      VK_CHECK(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.render));
    }
  }

  // init graphics pipelines
  {
    // Build pipeline layout that controls shader input/outputs
    auto pipelineLayoutInfo = vkinit::pipeline_layout_create_info();

    auto meshPipelineLayoutInfo = vkinit::pipeline_layout_create_info();
    VkPushConstantRange pushConstant{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .offset = 0,
      .size = sizeof MeshPushConstants,
    };
    meshPipelineLayoutInfo.pushConstantRangeCount = 1;
    meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
    VK_CHECK(vkCreatePipelineLayout(device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout));

    // Create graphics pipeline
    PipelineBuilder builder{
      .vertexInputInfo = vkinit::vertex_input_state_create_info(),
      .inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
      .viewport{ 0.f, 0.f, (float)window.get_width(), (float)window.get_height(), 0.f, 1.f },
      .scissor{
        .offset = { 0, 0 },
        .extent = { window.get_width(), window.get_height() }
      },
      .rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL),
      .colorBlendAttachment = vkinit::color_blend_attachment_state(),
      .depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL),
      .multisampling = vkinit::multisampling_state_create_info(),
      .pipelineLayout = pipelineLayout
    };

    // Triangle pipeline
    {
      VkShaderModule triVertShader, triFragShader;

      if (!vkinit::load_shader_module("shaders/colored_triangle.vert.spv", device, triVertShader))
        std::cout << "Failed to build colored triangle vertex shader\n";
    
      if (!vkinit::load_shader_module("shaders/colored_triangle.frag.spv", device, triFragShader))
        std::cout << "Failed to build colored triangle fragment shader\n";

      builder.shaderStages = {
        vkinit::shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triVertShader),
        vkinit::shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triFragShader),
      };

      trianglePipeline = builder.build_pipeline(device, renderPass);
      
      vkDestroyShaderModule(device, triVertShader, nullptr);
      vkDestroyShaderModule(device, triFragShader, nullptr);
    }

    // Red Triangle pipeline
    {
      VkShaderModule redTriVertShader, redTriFragShader;

      if (!vkinit::load_shader_module("shaders/triangle.vert.spv", device, redTriVertShader))
        std::cout << "Failed to build red triangle vertex shader\n";
    
      if (!vkinit::load_shader_module("shaders/triangle.frag.spv", device, redTriFragShader))
        std::cout << "Failed to build red triangle fragment shader\n";

      builder.shaderStages = {
        vkinit::shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, redTriVertShader),
        vkinit::shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, redTriFragShader),
      };

      redTrianglePipeline = builder.build_pipeline(device, renderPass);

      vkDestroyShaderModule(device, redTriVertShader, nullptr);
      vkDestroyShaderModule(device, redTriFragShader, nullptr);
    }

    // Mesh Pipeline
    {
      VertexInputDescription vid = Vertex::get_vertex_description();

      builder.vertexInputInfo.vertexAttributeDescriptionCount = vid.attributes.size();
      builder.vertexInputInfo.pVertexAttributeDescriptions = vid.attributes.data();
      builder.vertexInputInfo.vertexBindingDescriptionCount = vid.bindings.size();
      builder.vertexInputInfo.pVertexBindingDescriptions = vid.bindings.data();

      VkShaderModule meshVertShader, meshFragShader;

      if (!vkinit::load_shader_module("shaders/tri_mesh.vert.spv", device, meshVertShader))
        std::cout << "Failed to build triangle mesh vertex shader\n";
    
      if (!vkinit::load_shader_module("shaders/tri_mesh.frag.spv", device, meshFragShader))
        std::cout << "Failed to build triangle mesh fragment shader\n";

      builder.shaderStages = {
        vkinit::shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader),
        vkinit::shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, meshFragShader),
      };
      builder.pipelineLayout = meshPipelineLayout;

      meshPipeline = builder.build_pipeline(device, renderPass);

      create_material(meshPipeline, meshPipelineLayout, "defaultmesh");
      
      vkDestroyShaderModule(device, meshVertShader, nullptr);
      vkDestroyShaderModule(device, meshFragShader, nullptr);
    }
  }

  // init meshes
  {
    triangleMesh.vertices = {
      Vertex{.position{ 1.f, 1.f, 0.f }, .color{ 0.f, 1.f, 0.f }},
      Vertex{.position{-1.f, 1.f, 0.f }, .color{ 0.f, 1.f, 0.f }},
      Vertex{.position{ 0.f,-1.f, 0.f }, .color{ 0.f, 1.f, 0.f }}
    };

    std::filesystem::path p = std::filesystem::current_path() / "assets";
    monkeyMesh = load_from_obj(p.string() + "\\monkey_smooth.obj", p.string());
    thingMesh = load_from_obj(p.string() + "\\thing.obj", p.string());

    triangleMesh.upload(allocator);
    monkeyMesh.upload(allocator);
    thingMesh.upload(allocator);

    // Note that we are copying them. 
    // Eventually we will delete the hardcoded monkey and triangle meshes, so it's no problem now.
    meshes["monkey"] = monkeyMesh;
    meshes["triangle"] = triangleMesh;
    meshes["thing"] = thingMesh;
  }

  // init scene
  {
    RenderObject monkey{
      .mesh = get_mesh("monkey"),
      .mat = get_material("defaultmesh"),
      .transform = glm::mat4{ 1.f }
    };

    if(monkey.mesh && monkey.mat)
      objects.push_back(monkey);
    else
      std::cout << "mesh or mat not good lol\n";

    for (int x = -20; x <= 20; ++x)
    {
      for (int y = -20; y <= 20; ++y)
      {
        auto t = glm::translate(glm::mat4{ 1.f }, glm::vec3{ x, 0.f, y });
        auto s = glm::scale(glm::mat4{ 1.f }, glm::vec3{ .2f, .2f, .2f });

        RenderObject tri{
          .mesh = get_mesh("thing"),
          .mat = get_material("defaultmesh"),
          .transform = t * s
        };

        if (tri.mesh && tri.mat)
          objects.push_back(tri);
        else
          std::cout << "mesh or mat not good lol\n";
      }
    }
  }
}

void VulkanRenderer::draw(const Window& window)
{
  constexpr uint64_t timeout = std::numeric_limits<uint64_t>::max();
  const FrameData& frame = get_current_frame();

  VK_CHECK(vkWaitForFences(device, 1, &frame.fence, VK_TRUE, timeout));
	VK_CHECK(vkResetFences(device, 1, &frame.fence));

  uint32_t swapchainImageIndex;
  VK_CHECK(vkAcquireNextImageKHR(device, swapchain.get_swap_chain(), timeout, frame.present, nullptr, &swapchainImageIndex));

  // Now that rendering is finished for last frame, we can begin our rendering commands
  VK_CHECK(vkResetCommandBuffer(frame.cmdBuffer, 0));

  // Record our draw commands
  {
    VkCommandBufferBeginInfo cmdBegin{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = nullptr,

      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      .pInheritanceInfo = nullptr,
    };

    VK_CHECK(vkBeginCommandBuffer(frame.cmdBuffer, &cmdBegin));
    {
      VkClearValue clearColor{
        .color = {{ 0.f, 0.f, std::abs(std::sin(frameNumber / 120.f)), 1.f }}
      };

      VkClearValue depthClear{
        .depthStencil{ .depth = 1.f }
      };

      VkClearValue clearValues[2] = { clearColor, depthClear };

      VkRenderPassBeginInfo renderpassBegin{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,

        .renderPass = renderPass,
        .framebuffer = framebuffers[swapchainImageIndex],
        .renderArea{ 
          .offset{ .x = 0, .y = 0 },
          .extent{ swapchain.get_extents() }
        },

        .clearValueCount = 2,
        .pClearValues = clearValues,
      };

      vkCmdBeginRenderPass(frame.cmdBuffer, &renderpassBegin, VK_SUBPASS_CONTENTS_INLINE);

      draw_objects(frame.cmdBuffer, objects.data(), objects.size());

      vkCmdEndRenderPass(frame.cmdBuffer);
    }
    VK_CHECK(vkEndCommandBuffer(frame.cmdBuffer));
  }

  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit{
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext = nullptr,

    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &frame.present,
    .pWaitDstStageMask = &waitStage,

    .commandBufferCount = 1,
    .pCommandBuffers = &frame.cmdBuffer,

    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &frame.render,
  };

  VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, frame.fence));

  VkPresentInfoKHR presentInfo{
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = nullptr,

    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &frame.render,

    .swapchainCount = 1,
    .pSwapchains = &swapchain.get_swap_chain(),

    .pImageIndices = &swapchainImageIndex
  };

  VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

  frameNumber += 1;
}

void VulkanRenderer::swap_pipeline()
{
  shader ^= 1;
}

void VulkanRenderer::cleanup()
{
  constexpr auto timeout = 1000000000; // 1 second (in nanoseconds)

  for (int i = 0; i < MaxFramesInFlight; ++i)
  {
    vkWaitForFences(device, 1, &frames[i].fence, VK_TRUE, 1000000000);
    vkDestroyFence(device, frames[i].fence, nullptr);

    vkDestroySemaphore(device, frames[i].render, nullptr);
    vkDestroySemaphore(device, frames[i].present, nullptr);
    
    vkDestroyCommandPool(device, frames[i].cmdPool, nullptr);
  }

  vmaDestroyBuffer(allocator, triangleMesh.vertexBuffer.buffer, triangleMesh.vertexBuffer.alloc);
  vmaDestroyBuffer(allocator, monkeyMesh.vertexBuffer.buffer, monkeyMesh.vertexBuffer.alloc);
  vmaDestroyBuffer(allocator, thingMesh.vertexBuffer.buffer, thingMesh.vertexBuffer.alloc);

  vkDestroyImageView(device, depthImageView, nullptr);
  vmaDestroyImage(allocator, depthImage.image, depthImage.alloc);

  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);

  vkDestroyPipeline(device, trianglePipeline, nullptr);
  vkDestroyPipeline(device, redTrianglePipeline, nullptr);
  vkDestroyPipeline(device, meshPipeline, nullptr);

  vkDestroyRenderPass(device, renderPass, nullptr);
  for(const auto& framebuffer : framebuffers)
    vkDestroyFramebuffer(device, framebuffer, nullptr);

  swapchain.cleanup(device);
  
  vmaDestroyAllocator(allocator);

  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkb::destroy_debug_utils_messenger(instance, debugMessenger);
  vkDestroyInstance(instance, nullptr);
}

Material* VulkanRenderer::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
{
  auto [iter, success] = materials.insert({ name, { pipeline, layout } });
  if(success)
    return &iter->second;
  return nullptr;
}

Material* VulkanRenderer::get_material(const std::string& name)
{
  if (auto iter = materials.find(name); iter != materials.end())
    return &iter->second;
  return nullptr;
}

Mesh* VulkanRenderer::get_mesh(const std::string& name)
{
  if (auto iter = meshes.find(name); iter != meshes.end())
    return &iter->second;
  return nullptr;
}

void VulkanRenderer::draw_objects(VkCommandBuffer cmd, RenderObject* first, int count)
{
  glm::mat4 view = glm::lookAt(camPos, camPos + camFwd, glm::vec3{ 0.f, 1.f, 0.f });

  glm::mat4 projection = glm::perspective(glm::radians(70.f), 800.f / 600.f, 0.1f, 200.0f);
  projection[1][1] *= -1; // flip y axis for vulkan

  Mesh* lastMesh = nullptr;
  Material* lastMat = nullptr;
  for (int i = 0; i < count; ++i)
  {
    RenderObject& obj = first[i];

    if (!obj.mat && !obj.mesh)
      continue;

    // Only bind pipeline if pipeline is not currently bound
    if (obj.mat != lastMat)
    {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.mat->pipeline);
      lastMat = obj.mat;
    }

    MeshPushConstants constants{
      .render_matrix = projection * view * obj.transform
    };

    vkCmdPushConstants(cmd, obj.mat->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof MeshPushConstants, &constants);
    
    if (obj.mesh != lastMesh)
    {
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &obj.mesh->vertexBuffer.buffer, &offset);
      lastMesh = obj.mesh;
    }

    vkCmdDraw(cmd, obj.mesh->vertices.size(), 1, 0, 0);
  }
}

FrameData& VulkanRenderer::get_current_frame()
{
  return frames[frameNumber % MaxFramesInFlight];
}
