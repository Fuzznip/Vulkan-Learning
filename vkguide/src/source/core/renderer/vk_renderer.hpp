#pragma once

#include "core/window/window.hpp"
#include "core/renderer/vk_mesh.hpp"
#include "core/renderer/vk_pipeline.hpp"
#include "core/renderer/vk_swapchain.hpp"

constexpr uint32_t MaxFramesInFlight = 2;

// Camera data
struct MeshPushConstants
{
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct Material
{
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

struct RenderObject
{
  Mesh* mesh;
  Material* mat;
  glm::mat4 transform;
};

struct GPUCameraData
{
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
  alignas(16) glm::mat4 viewproj;
};

struct FrameData
{
  VkSemaphore present, render;
  VkFence fence;

  VkCommandPool cmdPool;
  VkCommandBuffer cmdBuffer;

  // Buffer that holds a GPUCameraData for use when rendering
  AllocatedBuffer cameraBuffer;
  VkDescriptorSet descriptor;
};

class VulkanRenderer
{
public:
  void init(const std::string& appName, const Window& window, bool enableValidationLayers);

  void draw(const Window& window);

  void swap_pipeline();

  void cleanup();
  
////
  glm::vec3 camPos{ 0.f, -6.f, -10.f };
  glm::vec3 camFwd{ 0.f, 0.f, -1.f };

private:
  Material* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
  Material* get_material(const std::string& name);
  Mesh* get_mesh(const std::string& name);

  void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);

  FrameData& get_current_frame();

  AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

  VmaAllocator allocator;

  VkInstance instance;
  VkPhysicalDevice gpu;
  VkDevice device;
  VkSurfaceKHR surface;
  VkPhysicalDeviceProperties gpuProperties;

  VulkanSwapchain swapchain;

  // Maybe part of render pass class
  VkImageView depthImageView;
  AllocatedImage depthImage;
  VkFormat depthFormat;

  VkQueue graphicsQueue;
  uint32_t graphicsQueueFamily;

  // Should this couple with swapchain? Need the imageviews to sync with framebuffers count
  // Or maybe its own render pass class that handles that stuff when we need to remake etc
  VkRenderPass renderPass;
  std::vector<VkFramebuffer> framebuffers;

  FrameData frames[MaxFramesInFlight];
  
  VkDescriptorSetLayout descriptorLayout;
  VkDescriptorPool descriptorPool;
   
  VkPipeline trianglePipeline;
	VkPipeline redTrianglePipeline;
  VkPipeline meshPipeline;
  Mesh triangleMesh;
  Mesh monkeyMesh;
  Mesh thingMesh;
  VkPipelineLayout pipelineLayout;
	VkPipelineLayout meshPipelineLayout;

  std::vector<RenderObject> objects;
  std::unordered_map<std::string, Material> materials;
  std::unordered_map<std::string, Mesh> meshes;

  VkDebugUtilsMessengerEXT debugMessenger; // Vulkan debug output handle

  uint64_t frameNumber = 0;

  int shader = 0;
};
