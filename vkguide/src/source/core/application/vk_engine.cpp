#include <pch.hpp>
#include "vk_engine.hpp"

#include "core/renderer/vk_initializers.hpp"
#include "core/renderer/vk_types.hpp"

void VulkanEngine::init()
{
  // We initialize SDL and create a window with it. 
  SDL_Init(SDL_INIT_VIDEO);

  window.init(name, 800, 600);

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
  double time = 0.0;

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
        if (e.key.keysym.sym == SDLK_SPACE)
          basicRenderer.swap_pipeline();
      } break;
      default:
        break;
      }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    draw();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
    auto frametime = ns.count() / 1000000000.0;
    
    time += frametime;

    if (time > 1.0)
    {
      window.set_window_title(fmt::format("{}: fps ({}ms) frametime ({:.4})", name, (int)(1.0 / frametime), frametime));
      time = 0.0;
    }
  }
}

void VulkanEngine::init_renderer()
{
  basicRenderer.init(name, window, true);
}

void VulkanEngine::draw()
{
  basicRenderer.draw(window);
}
