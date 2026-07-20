#version 460
#extension GL_EXT_nonuniform_qualifier : require


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

layout( push_constant ) uniform constants
{
	mat4 worldMatrix;
	uint padding[2];

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
