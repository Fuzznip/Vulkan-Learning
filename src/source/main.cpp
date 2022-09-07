#include <pch.hpp>

const uint32_t Width = 800;
const uint32_t Height = 600;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
  constexpr bool enableValidationLayers = false;
#else
  constexpr bool enableValidationLayers = true;
#endif

bool checkValidationLayerSupport()
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char* layerName : validationLayers) 
  {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers) 
    {
      if (strcmp(layerName, layerProperties.layerName) == 0) 
      {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) 
    {
      return false;
    }
  }

  return true;
}

std::vector<const char*> getRequiredExtensions(SDL_Window* window)
{
  unsigned int count = 0;
  if (!SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr)) 
    throw std::runtime_error("Failed to get SDL Vulkan extension count");

  std::vector<const char*> extensions = {
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME // Sample additional extension
  };
  const auto additionalExtensionCount = extensions.size();
  extensions.resize(additionalExtensionCount + count);
    
  if (!SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.data() + additionalExtensionCount)) 
    throw std::runtime_error("Failed to get SDL Vulkan extensions");

  if (enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr)
  {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  }
  else
  {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr)
  {
    func(instance, debugMessenger, pAllocator);
  }
}

class HelloTriangleApplication
{
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
  {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
      std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
  }

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete()
    {
      return graphicsFamily.has_value();
    }
  };

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
    setupDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
  }

  void createLogicalDevice()
  {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;

    // Vulkan lets you assign priorities to queues to influence the scheduling of command buffer execution using 
    // floating point numbers between 0.0 and 1.0. This is required even if there is only a single queue:
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;

    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    std::cout << "Logical Device created successfully!\n";

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    
    std::cout << "Graphics Queue retrieved successfully!\n";
  }

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
  {
    QueueFamilyIndices indices;

    // Assign index to queue families that could be found
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
      if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        indices.graphicsFamily = i;

      if (indices.isComplete())
        break;

      i++;
    }

    return indices;
  }

  bool isDeviceSuitable(VkPhysicalDevice device)
  {
    QueueFamilyIndices indices = findQueueFamilies(device);

    return indices.isComplete();
  }

  void pickPhysicalDevice()
  {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
      throw std::runtime_error("Failed to find GPU with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    std::cout << "\nPhysical Devices: " << devices.size() << '\n';
    for (const auto& device : devices)
    {
      VkPhysicalDeviceProperties properties;
      vkGetPhysicalDeviceProperties(device, &properties);
      std::cout << '\t' << "- " << properties.vendorID << ": " << properties.deviceName << '\n';
    }

    for (const auto& device : devices)
    {
      if (isDeviceSuitable(device))
      {
        physicalDevice = device;
        break;
      }
    }

    if (physicalDevice == VK_NULL_HANDLE)
      throw std::runtime_error("Failed to find a suitable GPU!");
    
    std::cout << "Physical Device selected successfully!\n";
  }

  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
  {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pNext = nullptr;
  }

  void setupDebugMessenger()
  {
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
      throw std::runtime_error("Failed to set up debug messenger!");
  }

  void createInstance()
  {
    if (enableValidationLayers && !checkValidationLayerSupport())
      throw std::runtime_error("Validation layers requested, but not available!");

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    appInfo.pNext = nullptr;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions(window);

    std::cout << "\nRequested Extensions: " << extensions.size() << '\n';
    for (const auto& extension : extensions)
      std::cout << '\t' << "- " << extension << '\n';

    // List available extensions
    {
      uint32_t extensionCount = 0;
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

      std::vector<VkExtensionProperties> extensions(extensionCount);
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
      std::cout << "\nAvailable extensions: " << extensions.size() <<'\n';

      for (const auto& extension : extensions) 
      {
        std::cout << '\t' << "- " << extension.extensionName << '\n';
      }
    }

    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers)
    {
      createInfo.enabledLayerCount = validationLayers.size();
      createInfo.ppEnabledLayerNames = validationLayers.data();

      std::cout << "\nValidation Layers Enabled:" << validationLayers.size() << '\n';
      for (const auto& validationLayer : validationLayers)
      {
        std::cout << '\t' << "- " << validationLayer << '\n';
      }
      
      populateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else
    {
      createInfo.enabledLayerCount = 0;

      createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
      throw std::runtime_error("Failed to create vkInstance!");
    
    std::cout << "Instance created successfully!\n";
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
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers)
    {
      DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

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
  VkDebugUtilsMessengerEXT debugMessenger;
  
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphicsQueue;
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
