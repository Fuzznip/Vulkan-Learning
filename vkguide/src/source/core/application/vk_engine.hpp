#pragma once

#include "core/renderer/vk_renderer.hpp"

class VulkanEngine 
{
public:
  void init();

  void cleanup();

  void run();

private:
  void init_renderer();

  void draw();

  const std::string name = "Vulkan Test Engine";

  Window window;
  VulkanRenderer basicRenderer;

  bool isInitialized = false;
  bool constrainMouse = false;
};