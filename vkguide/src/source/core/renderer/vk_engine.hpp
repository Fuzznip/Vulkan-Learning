// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "vk_types.hpp"

struct VulkanEngine 
{
  SDL_Window* window = nullptr;
	VkExtent2D windowExtent{ 1700 , 900 };

	bool isInitialized = false;

	int frameNumber = 0;

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();
};