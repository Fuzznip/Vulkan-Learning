#include <pch.hpp>

const uint32_t Width = 800;
const uint32_t Height = 600;

class HelloTriangleApplication
{
public:
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

private:
  void initWindow()
  {
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
      throw std::runtime_error(SDL_GetError());

    SDL_DisplayMode displayData;
    SDL_GetCurrentDisplayMode(0, &displayData);
  
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
    window = SDL_CreateWindow(
      "Vulkan",
      200, // pos
      200,
      Width, // size
      Height,
      flags
    );

    if (!window)
      throw std::runtime_error(SDL_GetError());
  }

  void initVulkan()
  {
    createInstance();
  }

  void createInstance()
  {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    unsigned int count;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr)) 
      throw std::runtime_error("Failed to get SDL Vulkan extension count");

    std::vector<const char*> extensions = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME // Sample additional extension
    };
    const auto additionalExtensionCount = extensions.size();
    extensions.resize(additionalExtensionCount + count);
    
    if (!SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.data() + additionalExtensionCount)) 
      throw std::runtime_error("Failed to get SDL Vulkan extensions");

    std::cout << "Requested Extensions: " << extensions.size() << '\n';
    for (const auto& extension : extensions)
      std::cout << '\t' << "- " << extension << '\n';

    // List available extensions
    {
      uint32_t extensionCount = 0;
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

      std::vector<VkExtensionProperties> extensions(extensionCount);
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
      std::cout << "Available extensions:\n";

      for (const auto& extension : extensions) 
      {
        std::cout << '\t' << "- " << extension.extensionName << '\n';
      }
    }

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
      throw std::runtime_error("Failed to create vkInstance!");
  }

  void mainLoop()
  {
    SDL_Event e;
	  bool running = true;

	  // main loop
	  while (running)
	  {
		  // Handle events on queue
		  while (SDL_PollEvent(&e) != 0)
		  {
			  // close the window when user clicks the X button or alt-f4s
			  if (e.type == SDL_QUIT) running = false;
		  }
	  }
  }

  void cleanup()
  {
    vkDestroyInstance(instance, nullptr);

    if (window)
    {
      SDL_DestroyWindow(window);
    }

    SDL_Quit();
  }

// Variables
private:
  SDL_Window* window = nullptr;

  VkInstance instance;
};

int main(int argc, char** argv) try
{
  HelloTriangleApplication{}.run();

  return EXIT_SUCCESS;
}
catch (const std::exception& e)
{
  std::cerr << e.what() << std::endl;
  return EXIT_FAILURE;
}
catch (...)
{
  std::cerr << "An unknown error has occurred" << std::endl;
  return EXIT_FAILURE;
}
