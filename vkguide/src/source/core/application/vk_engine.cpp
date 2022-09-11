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
  auto frametime = 0.016;
          
  float pitch = 0.0f, yaw = -90.0f;

  // main loop
  while (!quit)
  {
    auto t1 = std::chrono::high_resolution_clock::now();
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
      case SDL_KEYDOWN: {

      } break;
      case SDL_KEYUP: {
        if (e.key.keysym.sym == SDLK_SPACE)
        {
          constrainMouse ^= 1;
          SDL_SetRelativeMouseMode(constrainMouse ? SDL_TRUE : SDL_FALSE);
        }
      } break;
      case SDL_MOUSEMOTION: {
        if (constrainMouse && SDL_GetWindowFlags(window.window) & SDL_WindowFlags::SDL_WINDOW_INPUT_FOCUS)
        {
          float xDelta = e.motion.xrel;
          float yDelta = -e.motion.yrel;

          xDelta *= .3f;
          yDelta *= .3f;
          
          yaw += xDelta;
          pitch += yDelta;

          if (pitch > 89.0f)
            pitch = 89.0f;
          if (pitch < -89.0f)
            pitch = -89.0f;

          glm::vec3 direction;
          direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
          direction.y = sin(glm::radians(pitch));
          direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
          basicRenderer.camFwd = glm::normalize(direction);
        }
      } break;
      default:
        break;
      }
    }

    const uint8_t* keystates = SDL_GetKeyboardState(nullptr);

    auto camRight = glm::normalize(glm::cross(basicRenderer.camFwd, glm::vec3{ 0.f, 1.f, 0.f }));
    auto camUp = glm::normalize(glm::cross(basicRenderer.camFwd, camRight));

    if (keystates[SDL_SCANCODE_W])
      basicRenderer.camPos += (float)(15.f * frametime) * basicRenderer.camFwd;
    if (keystates[SDL_SCANCODE_A])
      basicRenderer.camPos -= (float)(15.f * frametime) * camRight;
    if (keystates[SDL_SCANCODE_S])
      basicRenderer.camPos -= (float)(15.f * frametime) * basicRenderer.camFwd;
    if (keystates[SDL_SCANCODE_D])
      basicRenderer.camPos += (float)(15.f * frametime) * camRight;
    if (keystates[SDL_SCANCODE_E])
      basicRenderer.camPos -= (float)(15.f * frametime) * camUp;
    if (keystates[SDL_SCANCODE_Q])
      basicRenderer.camPos += (float)(15.f * frametime) * camUp;

    draw();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
    frametime = ns.count() / 1000000000.0;
    
    time += frametime;

    if (time > 1.0)
    {
      window.set_window_title(fmt::format("{}: {} fps ({:.4}ms)", name, (int)(1.0 / frametime), frametime * 1000));
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
