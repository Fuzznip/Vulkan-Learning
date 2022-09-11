#version 460
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec3 color;

layout(location = 0) out vec3 outColor;

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

struct ObjectData
{
  mat4 model;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer
{
  ObjectData objects[];
} objectBuffer;

layout(push_constant) uniform constants
{
  vec4 data;
  mat4 render_matrix;
} PushConstants;

void main()
{
  mat4 transform = sceneData.camera.viewproj * objectBuffer.objects[gl_BaseInstance].model;
  gl_Position = transform * vec4(pos, 1.0f);
  outColor = color;
}
