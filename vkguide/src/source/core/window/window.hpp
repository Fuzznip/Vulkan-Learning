#pragma once

class Window
{
public:
  void init(const std::string& title, uint32_t w, uint32_t h);

  void set_window_title(const std::string& title);

  VkSurfaceKHR create_surface(VkInstance instance) const;

  uint32_t get_width() const { return width; }
  uint32_t get_height() const { return height; }

  void cleanup();

  SDL_Window* window;
private:
  uint32_t width, height;
};
