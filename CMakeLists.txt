cmake_minimum_required(VERSION 3.16)

project(Vulkan-Tutorial VERSION 1.0.0)

find_package(Vulkan REQUIRED FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)

set(SourceDirectory ${CMAKE_SOURCE_DIR}/src/source)
set(ExternalDirectory ${CMAKE_SOURCE_DIR}/src/external)

add_definitions(-DYAML_CPP_STATIC_DEFINE)

# Repos with CMakeLists
add_subdirectory(${ExternalDirectory}/SDL)
add_subdirectory(${ExternalDirectory}/spdlog)
add_subdirectory(${ExternalDirectory}/tinyobjloader)

# Header only with CMakeLists
add_library(GLM INTERFACE)

target_include_directories(GLM SYSTEM INTERFACE ${ExternalDirectory}/glm)

# Repos with no CMakeLists
add_library(IMGUI STATIC)

target_sources(IMGUI
  PRIVATE
    ${ExternalDirectory}/imgui/imgui_demo.cpp
    ${ExternalDirectory}/imgui/imgui_draw.cpp
    ${ExternalDirectory}/imgui/imgui_tables.cpp
    ${ExternalDirectory}/imgui/imgui_widgets.cpp
    ${ExternalDirectory}/imgui/imgui.cpp

  PRIVATE
    ${ExternalDirectory}/imgui/backends/imgui_impl_sdl.cpp
    ${ExternalDirectory}/imgui/backends/imgui_impl_vulkan.cpp
)

target_include_directories( IMGUI 
                            SYSTEM
                            PUBLIC ${ExternalDirectory}/imgui
                            PUBLIC ${ExternalDirectory}/imgui/backends
                            PUBLIC ${ExternalDirectory}/SDL/include
                            )

target_link_libraries(IMGUI PRIVATE SDL2main SDL2-static Vulkan::Vulkan)

# Include directories not added through other targets
include_directories(SYSTEM
  ${ExternalDirectory}/glm
  ${ExternalDirectory}/stb
  ${ExternalDirectory}/tinyobjloader)

# Include our source directory as include directory
include_directories(${SourceDirectory})

FILE(GLOB_RECURSE Source ${SourceDirectory}/*.cpp)
FILE(GLOB_RECURSE Header ${SourceDirectory}/*.h ${SourceDirectory}/*.hpp)

source_group(TREE ${SourceDirectory} FILES ${Source})
source_group(TREE ${SourceDirectory} FILES ${Header})

add_executable(Vulkan-Tutorial ${Source} ${Header})

target_link_libraries(Vulkan-Tutorial PUBLIC 
  SDL2main 
  SDL2-static 
  Vulkan::Vulkan 
  IMGUI
  spdlog::spdlog)

target_precompile_headers(Vulkan-Tutorial PUBLIC src/source/pch.hpp)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Vulkan-Tutorial)
