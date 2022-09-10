#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 inFragColor;

layout(location = 0) out vec4 outFragColor;

void main()
{
	outFragColor = vec4(inFragColor, 1.0);
}
