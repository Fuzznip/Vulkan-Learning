#include <pch.hpp>

#include "window.hpp"

void Window::init(const std::string& title, uint32_t w, uint32_t h)
{
  SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN;
  
  window = SDL_CreateWindow(
    title.c_str(),
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    w,
    h,
    window_flags
  );

  width = w;
  height = h;
}

void Window::set_window_title(const std::string& title)
{
  SDL_SetWindowTitle(window, title.c_str());
}

VkSurfaceKHR Window::create_surface(VkInstance instance) const
{
  VkSurfaceKHR surface;
  SDL_Vulkan_CreateSurface(window, instance, &surface);

  return surface;
}

void Window::cleanup()
{
  SDL_DestroyWindow(window);
}
