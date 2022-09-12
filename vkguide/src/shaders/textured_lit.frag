#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 outFragColor;

struct CameraData
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
};

struct Scene
{
	vec4 fogColor; // w is for exponent
	vec4 fogDistances; // x for min, y for max, zw unused.
	vec4 ambientColor;
	vec4 sunlightDirection; // w for sun power
	vec4 sunlightColor;
};

layout(set = 0, binding = 0) uniform SceneData
{
	CameraData camera;
	Scene scene;
} sceneData;

layout(set = 2, binding = 0) uniform sampler2D tex1;

void main()
{
	outFragColor = vec4(texture(tex1, uv).xyz, 1.f);
}
