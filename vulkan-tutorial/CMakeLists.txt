cmake_minimum_required(VERSION 3.16)

project(Vulkan-Tutorial VERSION 1.0.0)

find_package(Vulkan REQUIRED FATAL_ERROR glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)

set(SourceDirectory ${CMAKE_SOURCE_DIR}/src/source)
set(ShaderDirectory ${CMAKE_SOURCE_DIR}/src/shaders)
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

# Compile shaders on build
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

FILE(GLOB_RECURSE Shaders
  ${SHADER_SOURCE_DIR}/*.vert
  ${SHADER_SOURCE_DIR}/*.frag
  ${SHADER_SOURCE_DIR}/*.comp
  ${SHADER_SOURCE_DIR}/*.geom
  ${SHADER_SOURCE_DIR}/*.tesc
  ${SHADER_SOURCE_DIR}/*.tese
  ${SHADER_SOURCE_DIR}/*.mesh
  ${SHADER_SOURCE_DIR}/*.task
  ${SHADER_SOURCE_DIR}/*.rgen
  ${SHADER_SOURCE_DIR}/*.rchit
  ${SHADER_SOURCE_DIR}/*.rmiss
)

add_custom_command(
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
  OUTPUT ${SHADER_BINARY_DIR}
  COMMENT "Creating ${SHADER_BINARY_DIR}"
)

foreach(source IN LISTS Shaders)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${glslc_executable}
      #      -MD -MF ${SHADER_BINARY_DIR}/${FILENAME}.d
      -o ${SHADER_BINARY_DIR}/${FILENAME}.spv
      ${source}
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source} ${SHADER_BINARY_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

# Include our source directory as include directory
include_directories(${SourceDirectory})
include_directories(${ShaderDirectory})

FILE(GLOB_RECURSE Source ${SourceDirectory}/*.cpp ${SourceDirectory}/*.h ${SourceDirectory}/*.hpp)

source_group(TREE ${SourceDirectory} FILES ${Source})
source_group(TREE ${ShaderDirectory} FILES ${Shaders})

add_executable(Vulkan-Tutorial ${Source} ${Shaders})

# Symlink our assets folder to our build folder
set (source "${CMAKE_SOURCE_DIR}/assets")
set (destination "${CMAKE_CURRENT_BINARY_DIR}/assets")
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination} 
  DEPENDS ${destination}
  COMMENT "symbolic link resources folder from ${source} => ${destination}"
)

target_link_libraries(Vulkan-Tutorial PUBLIC 
  SDL2main 
  SDL2-static 
  Vulkan::Vulkan 
  IMGUI
  spdlog::spdlog)

target_precompile_headers(Vulkan-Tutorial PUBLIC src/source/pch.hpp)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Vulkan-Tutorial)
