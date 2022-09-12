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
  VkDescriptorSet texture = VK_NULL_HANDLE;
};

struct Texture
{
  AllocatedImage image;
  VkImageView view;
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

struct GPUSceneData
{
	alignas(16) glm::vec4 fogColor; // w is for exponent
	alignas(16) glm::vec4 fogDistances; // x for min, y for max, zw unused.
	alignas(16) glm::vec4 ambientColor;
	alignas(16) glm::vec4 sunlightDirection; // w for sun power
	alignas(16) glm::vec4 sunlightColor;
};

struct GPUObjectData
{
	alignas(16) glm::mat4 model;
};

struct FrameData
{
  VkSemaphore present, render;
  VkFence fence;

  VkCommandPool cmdPool;
  VkCommandBuffer cmdBuffer;

  AllocatedBuffer objectBuffer;
  VkDescriptorSet objectDescriptor;
};

struct UploadContext
{
  VkFence upload;
  VkCommandPool pool;
  VkCommandBuffer buffer;
};

class VulkanRenderer
{
public:
  void init(const std::string& appName, const Window& window, bool enableValidationLayers);

  void draw(double dt);

  void swap_pipeline();

  void cleanup();
  
////
  AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

  void immediate_submit(std::function<void(VkCommandBuffer)>&& func);

  VmaAllocator allocator;

  glm::vec3 camPos{ 0.f, -6.f, -10.f };
  glm::vec3 camFwd{ 0.f, 0.f, -1.f };

private:
  Material* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
  Material* get_material(const std::string& name);
  Mesh* get_mesh(const std::string& name);
  void upload_mesh(Mesh& mesh);

  void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);

  FrameData& get_current_frame();
  size_t pad_uniform_buffer_size(size_t originalSize) const;

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

  // Buffer that holds a GPUCameraData for use when rendering
  GPUSceneData scene;
  AllocatedBuffer sceneBuffer;
  VkDescriptorSet sceneDescriptor;
  
	VkSampler blockySampler;
  
  VkDescriptorSetLayout descriptorLayout;
  VkDescriptorSetLayout objectSetLayout;
  VkDescriptorSetLayout singleTextureSetLayout;
  VkDescriptorPool descriptorPool;
   
  VkPipeline trianglePipeline;
	VkPipeline redTrianglePipeline;
  VkPipeline meshPipeline;
  VkPipeline texturedPipeline;
  Mesh triangleMesh;
  Mesh monkeyMesh;
  Mesh thingMesh;
  VkPipelineLayout pipelineLayout;
	VkPipelineLayout meshPipelineLayout;
	VkPipelineLayout texturedPipelineLayout;

  std::vector<RenderObject> objects;
  std::unordered_map<std::string, Material> materials;
  std::unordered_map<std::string, Mesh> meshes;
  std::unordered_map<std::string, Texture> textures;

  UploadContext upload;

  VkDebugUtilsMessengerEXT debugMessenger; // Vulkan debug output handle
  
  const uint64_t timeout = 1000000000; // 1 second

  uint64_t frameNumber = 0;
  double t = 0;

  int shader = 0;
};
