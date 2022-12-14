#include <pch.hpp>
#include "core/renderer/vk_renderer.hpp"

#include "core/renderer/vk_initializers.hpp"
#include "core/renderer/vk_textures.hpp"
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

  VkPhysicalDeviceShaderDrawParameterFeatures shaderDrawParamFeatures{
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
    .pNext = nullptr,

    .shaderDrawParameters = VK_TRUE,
  };

  vkb::Device gpuDevice = vkb::DeviceBuilder{ selectedGpu }.add_pNext(&shaderDrawParamFeatures).build().value();

  gpu = selectedGpu.physical_device;
  gpuProperties = selectedGpu.properties;
  std::cout << selectedGpu.name << '\n';
  std::cout << "The GPU has a minimum buffer alignment of " << gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
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

    auto uploadPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily);

    VK_CHECK(vkCreateCommandPool(device, &uploadPoolInfo, nullptr, &upload.pool));

    auto uploadAllocInfo = vkinit::command_buffer_allocate_info(upload.pool, 1);

    VK_CHECK(vkAllocateCommandBuffers(device, &uploadAllocInfo, &upload.buffer));
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

    VkFenceCreateInfo uploadFenceInfo{
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = nullptr,

      .flags = 0
    };

    VK_CHECK(vkCreateFence(device, &uploadFenceInfo, nullptr, &upload.upload));
  }
  
  // init descriptors
  {
    // Create descriptor pool with 10 uniform buffers
    std::vector<VkDescriptorPoolSize> sizes{
      VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
      VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
      VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },

      VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
    };

    // Uniform buffer binding
    VkDescriptorSetLayoutBinding camBinding = vkinit::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);

    VkDescriptorSetLayoutBinding bindings[] = { camBinding };

    // All our bindings that go into our set
    VkDescriptorSetLayoutCreateInfo setInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,

      .flags = 0,
      .bindingCount = 1,
      .pBindings = bindings,
    };

    VK_CHECK(vkCreateDescriptorSetLayout(device, &setInfo, nullptr, &descriptorLayout));

    VkDescriptorSetLayoutBinding objBinding = vkinit::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    
    VkDescriptorSetLayoutCreateInfo objSetInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,

      .flags = 0,
      .bindingCount = 1,
      .pBindings = &objBinding
    };

    VK_CHECK(vkCreateDescriptorSetLayout(device, &objSetInfo, nullptr, &objectSetLayout));
    
    auto textureBinding = vkinit::descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

    VkDescriptorSetLayoutCreateInfo textureSetCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,

      .flags = 0,
      .bindingCount = 1,
      .pBindings = &textureBinding
    };

    VK_CHECK(vkCreateDescriptorSetLayout(device, &textureSetCreateInfo, nullptr, &singleTextureSetLayout));

    VkDescriptorPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,

      .flags = 0,
      .maxSets = 10,
      .poolSizeCount = (uint32_t)sizes.size(),
      .pPoolSizes = sizes.data(),
    };

    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool));

    const auto sceneBufSize = MaxFramesInFlight * (pad_uniform_buffer_size(sizeof GPUCameraData + sizeof GPUSceneData));
    sceneBuffer = create_buffer(sceneBufSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    
    VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,

      .descriptorPool = descriptorPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &descriptorLayout
    };

    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &sceneDescriptor));
    
    // Now populate descriptor set binding 0 to point to our scene buffer
    VkDescriptorBufferInfo sceneInfo{
      .buffer = sceneBuffer.buffer,
      .offset = 0,
      .range = sizeof GPUCameraData + sizeof GPUSceneData,
    };

    VkWriteDescriptorSet sceneWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, sceneDescriptor, &sceneInfo, 0);

    for (int i = 0; i < MaxFramesInFlight; ++i)
    {
      constexpr int MaxObjects = 10000; // Small count but it's just for example.
      frames[i].objectBuffer = create_buffer(sizeof GPUObjectData * MaxObjects, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

      VkDescriptorSetAllocateInfo objAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,

        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &objectSetLayout
      };

      VK_CHECK(vkAllocateDescriptorSets(device, &objAllocInfo, &frames[i].objectDescriptor));
      
      // Now populate descriptor set binding 1 to point to our scene buffer
      VkDescriptorBufferInfo objInfo{
        .buffer = frames[i].objectBuffer.buffer,
        .offset = 0,
        .range = sizeof GPUObjectData * MaxObjects,
      };

      VkWriteDescriptorSet objWrite = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, frames[i].objectDescriptor, &objInfo, 0);

      VkWriteDescriptorSet writes[] = { sceneWrite, objWrite };

      vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
    }
  }

  // init graphics pipelines
  {
    // Build pipeline layout that controls shader input/outputs
    auto pipelineLayoutInfo = vkinit::pipeline_layout_create_info();

    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

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
      auto meshPipelineLayoutInfo = vkinit::pipeline_layout_create_info();
      VkPushConstantRange pushConstant{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof MeshPushConstants,
      };
      meshPipelineLayoutInfo.pPushConstantRanges = &pushConstant;
      meshPipelineLayoutInfo.pushConstantRangeCount = 1;

      VkDescriptorSetLayout setLayouts[] = { descriptorLayout, objectSetLayout };

      meshPipelineLayoutInfo.pSetLayouts = setLayouts;
      meshPipelineLayoutInfo.setLayoutCount = 2;

      VK_CHECK(vkCreatePipelineLayout(device, &meshPipelineLayoutInfo, nullptr, &meshPipelineLayout));

      VertexInputDescription vid = Vertex::get_vertex_description();

      builder.vertexInputInfo.vertexAttributeDescriptionCount = vid.attributes.size();
      builder.vertexInputInfo.pVertexAttributeDescriptions = vid.attributes.data();
      builder.vertexInputInfo.vertexBindingDescriptionCount = vid.bindings.size();
      builder.vertexInputInfo.pVertexBindingDescriptions = vid.bindings.data();

      VkShaderModule meshVertShader, meshFragShader;

      if (!vkinit::load_shader_module("shaders/tri_mesh.vert.spv", device, meshVertShader))
        std::cout << "Failed to build mesh vertex shader\n";

      if (!vkinit::load_shader_module("shaders/default_lit.frag.spv", device, meshFragShader))
        std::cout << "Failed to build mesh fragment shader\n";

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

    // Textured Mesh Pipeline
    {
      auto texturedPipelineLayoutInfo = vkinit::pipeline_layout_create_info();
      VkPushConstantRange pushConstant{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof MeshPushConstants,
      };
      texturedPipelineLayoutInfo.pPushConstantRanges = &pushConstant;
      texturedPipelineLayoutInfo.pushConstantRangeCount = 1;

      VkDescriptorSetLayout setLayouts[] = { descriptorLayout, objectSetLayout, singleTextureSetLayout };

      texturedPipelineLayoutInfo.pSetLayouts = setLayouts;
      texturedPipelineLayoutInfo.setLayoutCount = 3;

      VK_CHECK(vkCreatePipelineLayout(device, &texturedPipelineLayoutInfo, nullptr, &texturedPipelineLayout));

      VertexInputDescription vid = Vertex::get_vertex_description();

      builder.vertexInputInfo.vertexAttributeDescriptionCount = vid.attributes.size();
      builder.vertexInputInfo.pVertexAttributeDescriptions = vid.attributes.data();
      builder.vertexInputInfo.vertexBindingDescriptionCount = vid.bindings.size();
      builder.vertexInputInfo.pVertexBindingDescriptions = vid.bindings.data();
      
      VkShaderModule textureVertShader, textureFragShader;

      if (!vkinit::load_shader_module("shaders/tri_mesh.vert.spv", device, textureVertShader))
        std::cout << "Failed to build textured mesh vertex shader\n";

      if (!vkinit::load_shader_module("shaders/textured_lit.frag.spv", device, textureFragShader))
        std::cout << "Failed to build textured mesh fragment shader\n";

      builder.shaderStages = {
        vkinit::shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, textureVertShader),
        vkinit::shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, textureFragShader),
      };
      builder.pipelineLayout = texturedPipelineLayout;

      texturedPipeline = builder.build_pipeline(device, renderPass);

      create_material(texturedPipeline, texturedPipelineLayout, "texturedmesh");

      vkDestroyShaderModule(device, textureVertShader, nullptr);
      vkDestroyShaderModule(device, textureFragShader, nullptr);
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
    auto empire = load_from_obj(p.string() + "\\lost_empire.obj", p.string());

    upload_mesh(triangleMesh);
    upload_mesh(monkeyMesh);
    upload_mesh(thingMesh);
    upload_mesh(empire);

    // Note that we are copying them. 
    // Eventually we will delete the hardcoded monkey and triangle meshes, so it's no problem now.
    meshes["monkey"] = monkeyMesh;
    meshes["triangle"] = triangleMesh;
    meshes["thing"] = thingMesh;
    meshes["empire"] = empire;
  }

  // init textures
  {
    Texture tex;
    
    if(!vkutil::load_image(*this, "assets\\lost_empire-RGBA.png", tex.image))
      std::cout << "bruh\n";

    auto imageInfo = vkinit::image_view_create_info(VK_FORMAT_R8G8B8A8_SRGB, tex.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device, &imageInfo, nullptr, &tex.view);

    textures["empire_diffuse"] = tex;
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

    RenderObject map{
      .mesh = get_mesh("empire"),
      .mat = get_material("texturedmesh"),
      .transform = glm::translate(glm::vec3{ 5, -10, 0 })
    };

    objects.push_back(map);

    VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);
	  vkCreateSampler(device, &samplerInfo, nullptr, &blockySampler);

    Material* mat = get_material("texturedmesh");

    VkDescriptorSetAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,

      .descriptorPool = descriptorPool,
      .descriptorSetCount = 1,
      .pSetLayouts = &singleTextureSetLayout
    };
    
	  vkAllocateDescriptorSets(device, &allocInfo, &mat->texture);

    VkDescriptorImageInfo imageBufferInfo{
      .sampler = blockySampler,
      .imageView = textures["empire_diffuse"].view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    auto tex = vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mat->texture, &imageBufferInfo, 0);

    vkUpdateDescriptorSets(device, 1, &tex, 0, nullptr);
  }
}

void VulkanRenderer::draw(double dt)
{
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
        .color = {{ 0.f, 0.f, std::abs(std::sin((float)t / 120.f)), 1.f }}
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
  t += dt;
}

void VulkanRenderer::swap_pipeline()
{
  shader ^= 1;
}

void VulkanRenderer::cleanup()
{
  for (int i = 0; i < MaxFramesInFlight; ++i)
  {
    vkWaitForFences(device, 1, &frames[i].fence, VK_TRUE, timeout);
    vkDestroyFence(device, frames[i].fence, nullptr);

    vkDestroySemaphore(device, frames[i].render, nullptr);
    vkDestroySemaphore(device, frames[i].present, nullptr);
    
    vkDestroyCommandPool(device, frames[i].cmdPool, nullptr);
  }

  // No need to wait since this is used for immediate pushes and is waited on immediately anyways
  vkDestroyFence(device, upload.upload, nullptr);

  vkDestroyCommandPool(device, upload.pool, nullptr);

  for (const auto& [str, t] : textures)
  {
    vkDestroyImageView(device, t.view, nullptr);
    vmaDestroyImage(allocator, t.image.image, t.image.alloc);
  }

  for (const auto& [str, m] : meshes)
    vmaDestroyBuffer(allocator, m.vertexBuffer.buffer, m.vertexBuffer.alloc);

  vkDestroyImageView(device, depthImageView, nullptr);
  vmaDestroyImage(allocator, depthImage.image, depthImage.alloc);

  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyPipelineLayout(device, meshPipelineLayout, nullptr);
  vkDestroyPipelineLayout(device, texturedPipelineLayout, nullptr);

  vkDestroyPipeline(device, trianglePipeline, nullptr);
  vkDestroyPipeline(device, redTrianglePipeline, nullptr);
  vkDestroyPipeline(device, meshPipeline, nullptr);
  vkDestroyPipeline(device, texturedPipeline, nullptr);

  for (int i = 0; i < MaxFramesInFlight; ++i)
    vmaDestroyBuffer(allocator, frames[i].objectBuffer.buffer, frames[i].objectBuffer.alloc);
  vmaDestroyBuffer(allocator, sceneBuffer.buffer, sceneBuffer.alloc);

  vkDestroySampler(device, blockySampler, nullptr);

  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, objectSetLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, singleTextureSetLayout, nullptr);

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

void VulkanRenderer::upload_mesh(Mesh& mesh)
{
  const uint32_t bufferSize = mesh.vertices.size() * sizeof Vertex;

  VkBufferCreateInfo stagingBufferInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,

    .size = bufferSize,
    .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
  };

  VmaAllocationCreateInfo stagingAllocInfo{
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO,
  };

  AllocatedBuffer tempStagingBuffer;

  // Create staging buffer
  VK_CHECK(vmaCreateBuffer(
    allocator,
    &stagingBufferInfo,
    &stagingAllocInfo,
    &tempStagingBuffer.buffer,
    &tempStagingBuffer.alloc,
    nullptr
  ));

  // Now that we have a buffer, copy data over into that buffer
  void* data;
  vmaMapMemory(allocator, tempStagingBuffer.alloc, &data);
  memcpy(data, mesh.vertices.data(), bufferSize);
  vmaUnmapMemory(allocator, tempStagingBuffer.alloc);

  // Now transfer data over to GPU buffer
  VkBufferCreateInfo gpuBufferInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,

    .size = bufferSize,
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
  };

  VmaAllocationCreateInfo gpuAllocInfo{
    .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
    .usage = VMA_MEMORY_USAGE_AUTO
  };

  // Create staging buffer
  VK_CHECK(vmaCreateBuffer(
    allocator,
    &gpuBufferInfo,
    &gpuAllocInfo,
    &mesh.vertexBuffer.buffer,
    &mesh.vertexBuffer.alloc,
    nullptr
  ));

  immediate_submit([=](VkCommandBuffer cmd){
    VkBufferCopy copy{
      .srcOffset = 0,
      .dstOffset = 0,
      .size = bufferSize,
    };

    vkCmdCopyBuffer(cmd, tempStagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &copy);
  });

  vmaDestroyBuffer(allocator, tempStagingBuffer.buffer, tempStagingBuffer.alloc);
}

void VulkanRenderer::draw_objects(VkCommandBuffer cmd, RenderObject* first, int count)
{
  glm::mat4 view = glm::lookAt(camPos, camPos + camFwd, glm::vec3{ 0.f, 1.f, 0.f });

  glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
  projection[1][1] *= -1; // flip y axis for vulkan

  GPUCameraData cam{
    .view = view,
    .proj = projection,
    .viewproj = projection * view,
  };

	scene.ambientColor = { sin(t), 0.f, cos(t), 1.f };

	int frameIndex = frameNumber % MaxFramesInFlight;

  uint8_t* data;
  vmaMapMemory(allocator, sceneBuffer.alloc, (void**)&data);

  // Not sure at all how this needs to be offset but this works now
  // (but so does using pad uniform buffer size...)

  // cam + scene data = 272 bytes
  // Cam Pad = 256
  const int camData = pad_uniform_buffer_size(sizeof GPUCameraData);
  // Scene Pad = 256
  const int sceneData = pad_uniform_buffer_size(sizeof GPUSceneData);
  // Cam + Scene Pad = 512
  const int camAndSceneData = pad_uniform_buffer_size(sizeof GPUCameraData + sizeof GPUSceneData);
  data += (sizeof GPUCameraData + sizeof GPUSceneData) * frameIndex;
  memcpy(data, &cam, sizeof GPUCameraData);
  
  // Move ptr to Scene Data beginning
	data += sizeof GPUCameraData;
  memcpy(data, &scene, sizeof GPUSceneData);

	vmaUnmapMemory(allocator, sceneBuffer.alloc);

  void* objectData;
  vmaMapMemory(allocator, get_current_frame().objectBuffer.alloc, &objectData);

  GPUObjectData* objectSSBO = reinterpret_cast<GPUObjectData*>(objectData);

  for (int i = 0; i < count; ++i)
  {
    RenderObject& obj = first[i];
    objectSSBO[i].model = obj.transform;
  }
  
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

      // Bind descriptor set when changing pipelines
      // Get uniform offset due to 1 dynamic descriptor set
      uint32_t dynamicOffset = 0;
      // Need to send 1 offset uint32_t for each dynamic descriptor we have
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.mat->layout, 0, 1, &sceneDescriptor, 1, &dynamicOffset);

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.mat->layout, 1, 1, &get_current_frame().objectDescriptor, 0, nullptr);

      if(obj.mat->texture != VK_NULL_HANDLE)
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.mat->layout, 2, 1, &obj.mat->texture, 0, nullptr);
    }

    MeshPushConstants constants{
      .render_matrix = obj.transform
    };

    vkCmdPushConstants(cmd, obj.mat->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof MeshPushConstants, &constants);
    
    if (obj.mesh != lastMesh)
    {
      VkDeviceSize offset = 0;
      vkCmdBindVertexBuffers(cmd, 0, 1, &obj.mesh->vertexBuffer.buffer, &offset);
      lastMesh = obj.mesh;
    }

    // first instance is 'i' so that we get our gl_BaseInstance set in vertex shader
    vkCmdDraw(cmd, obj.mesh->vertices.size(), 1, 0, i);
  }

  vmaUnmapMemory(allocator, get_current_frame().objectBuffer.alloc);
}

FrameData& VulkanRenderer::get_current_frame()
{
  return frames[frameNumber % MaxFramesInFlight];
}

AllocatedBuffer VulkanRenderer::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
  VkBufferCreateInfo bufInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = nullptr,

    .size = allocSize,
    .usage = usage
  };

  VmaAllocationCreateInfo allocInfo{
    .usage = memoryUsage
  };

  AllocatedBuffer buf;
  VK_CHECK(vmaCreateBuffer(allocator, &bufInfo, &allocInfo, &buf.buffer, &buf.alloc, nullptr));

  return buf;
}

size_t VulkanRenderer::pad_uniform_buffer_size(size_t originalSize) const
{
	// Calculate required alignment based on minimum device offset alignment
	const size_t minUboAlignment = gpuProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0) 
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	return alignedSize;
}

void VulkanRenderer::immediate_submit(std::function<void(VkCommandBuffer)>&& func)
{
  auto cmd = upload.buffer;

  // We will use this command buffer exactly once before resetting, so we tell Vulkan that
  auto beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

  func(cmd);

  VK_CHECK(vkEndCommandBuffer(cmd));

  auto submit = vkinit::submit_info(&cmd);

  VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, upload.upload));

  vkWaitForFences(device, 1, &upload.upload, VK_TRUE, timeout);
  vkResetFences(device, 1, &upload.upload);

  vkResetCommandPool(device, upload.pool, 0);
}
