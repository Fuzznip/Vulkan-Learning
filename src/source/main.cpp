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
    if (window)
    {
      SDL_DestroyWindow(window);
    }

    SDL_Quit();
  }

  SDL_Window* window = nullptr;
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
