#version 460
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outWorldPos;
layout (location = 4) out vec4 outTangent;

layout(set = 0, binding = 0) uniform SceneData {
	mat4 view;
	mat4 proj;
	mat4 viewproj;

	// Направленный источник света
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;

	// Тени
	mat4 cascadeMatrices[4]; // Матрицы света для 4 каскадов
	vec4 cascadeSplits;      // Дистанции разделения каскадов упакованы в vec4 (x, y, z, w)
	uint shadowMapTextureID;
} scene;

struct Vertex {

	vec3 position; float uv_x;
	vec3 normal; float uv_y;
	vec4 color;
	vec4 tangent;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	VertexBuffer vertexBuffer;

	uint colorTextureID;
	uint metallicRoughnessTextureID;
	uint normalTextureID;
	uint occlusionTextureID;

	vec2 padding;

	vec4 baseColorFactor;
	vec4 materialFactors; // x: roughness, y: metallic, z: emissive, w: padding
} PushConstants;

void main()
{
	//load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	vec4 worldPos = PushConstants.render_matrix * vec4(v.position, 1.0f);
	outWorldPos = worldPos.xyz;

	//output data
	gl_Position = scene.viewproj * PushConstants.render_matrix *vec4(v.position, 1.0f);

	mat3 normalMatrix = mat3(PushConstants.render_matrix);

	// Для мап нормалей
    outNormal = normalMatrix * v.normal;
    outTangent = vec4(normalMatrix * v.tangent.xyz, v.tangent.w);

	// Для альбедо текстур
    outUV = vec2(v.uv_x, v.uv_y);
    outColor = v.color;

    gl_Position = scene.viewproj * worldPos;
}
