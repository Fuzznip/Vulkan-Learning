#pragma once

// SDL
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

// Vulkan
#include <vulkan/vulkan.h>

// stb
#include <stb_image.h>

// tiny obj loader
#include <tiny_obj_loader.h>

// spdlog
#include <spdlog/spdlog.h>

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
