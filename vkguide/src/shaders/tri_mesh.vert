#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec3 color;

layout(location = 0) out vec3 outColor;

layout(push_constant) uniform constants
{
  vec4 data;
  mat4 render_matrix;
} PushConstants;

void main()
{
  gl_Position = PushConstants.render_matrix * vec4(pos, 1.0f);
  outColor = color;
}
