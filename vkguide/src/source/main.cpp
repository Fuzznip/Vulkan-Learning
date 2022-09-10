#include <pch.hpp>

#include "core/renderer/vk_engine.hpp"

int main(int argc, char** argv) try
{
  VulkanEngine engine;

  engine.init();

  engine.run();

  engine.cleanup();

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