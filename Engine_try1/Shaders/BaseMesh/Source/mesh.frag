#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform SceneData {
	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;
} scene;

// НАШ BINDLESS SET!
layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

struct Vertex {
	vec3 position; float uv_x;
	vec3 normal;   float uv_y;
	vec4 color;
};
layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{
	mat4 worldMatrix;
	VertexBuffer vertexBuffer;

	uint colorTextureID;
	uint metallicRoughnessTextureID;
} PushConstants;

void main() 
{
	uint texID = nonuniformEXT(PushConstants.colorTextureID);
	vec4 texColor = texture(globalTextures[texID], inUV);
	vec4 finalAlbedo = texColor * inColor;

	 outFragColor = finalAlbedo;
}
