#include <pch.hpp>

int main(int argc, char** argv)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    throw std::runtime_error(SDL_GetError());
  

  SDL_DisplayMode displayData;
  SDL_GetCurrentDisplayMode(0, &displayData);
  
  Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
  SDL_Window* window = SDL_CreateWindow(
    "Vulkan Window",
    200, // pos
    200,
    800, // size
    600,
    flags
  );

  if (!window)
    throw std::runtime_error(SDL_GetError());

  SDL_SetWindowSize(window, 800, 600);
  //SDL_SetWindowFullscreen(window, 0);

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  std::cout << extensionCount << " extensions supported\n";

  glm::mat4 matrix;
  glm::vec4 vec;
  auto test = matrix * vec;

  bool running = true;
  while (running)
  {
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_WINDOWEVENT:
        switch (event.window.event)
        {
        case SDL_WINDOWEVENT_CLOSE:
          running = false;
        }
      }
    }
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
