#include <pch.hpp>

#include "vk_engine.hpp"

#include "core/renderer/vk_initializers.hpp"
#include "core/renderer/vk_types.hpp"

void VulkanEngine::init()
{
  // We initialize SDL and create a window with it. 
  SDL_Init(SDL_INIT_VIDEO);

  window.init(name, 1700, 900);

  init_renderer();
  
  // everything went fine
  isInitialized = true;
}

void VulkanEngine::cleanup()
{
  if (isInitialized)
  {
    basicRenderer.cleanup();
    window.cleanup();
  }
}

void VulkanEngine::run()
{
  SDL_Event e;
  bool quit = false;

  // main loop
  while (!quit)
  {
    // Handle events on queue
    while (SDL_PollEvent(&e) != 0)
    {
      switch (e.type)
      {
      // close the window when user clicks the X button or alt-f4s
      case SDL_QUIT: {
        quit = true;
      } break;
      case SDL_WINDOWEVENT: {
        switch (e.window.event)
        {
        case SDL_WINDOWEVENT_RESIZED: {
        } break;
        case SDL_WINDOWEVENT_MINIMIZED: {
        } break;
        case SDL_WINDOWEVENT_EXPOSED:
        case SDL_WINDOWEVENT_RESTORED: {
        } break;
        default:
          break;
        }
      } break;
      case SDL_KEYUP: {

      } break;
      default:
        break;
      }
    }

    draw();
  }
}

void VulkanEngine::init_renderer()
{
  basicRenderer.init(name, window, true);
}

void VulkanEngine::draw()
{
  // nothing yet
}
