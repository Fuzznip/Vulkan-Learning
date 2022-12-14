#pragma once

#pragma warning (disable:6011)  // Dereferencing NULL pointer
#pragma warning (disable:6237)  // Zero and expression is always zero. Expression is never evaluated
#pragma warning (disable:26495) // Member variables must always be initialized
#pragma warning (disable:26812) // Prefer Enum Class over Enum

// SDL
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

// fmt
#include <fmt/format.h>

// stb
#include <stb_image.h>

// tiny obj loader
#include <tiny_obj_loader.h>

// Vulkan
#include <vulkan/vulkan.h>

// vk-bootstrap
#include <VkBootstrap.h>

// VulkanMemoryAllocator (VMA)
#include <vk_mem_alloc.h>

// C headers
#include <cassert>
#include <cmath>

// STL includes
#include <algorithm>
#include <any>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <vector>

using namespace std::chrono_literals;
