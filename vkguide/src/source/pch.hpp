#pragma once

#pragma warning (disable:6237)  // Zero and expression is always zero. Expression is never evaluated
#pragma warning (disable:26495) // Member variables must always be initialized
#pragma warning (disable:26812) // Prefer Enum Class over Enum

// SDL
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_vulkan.h>

// Vulkan
#include <vulkan/vulkan.h>

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
