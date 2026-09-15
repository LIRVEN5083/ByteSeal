#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_gpu_shader_int64 : require

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outWorldPos;
layout (location = 4) out vec4 outTangent;

layout (location = 5) out vec4 outCurrentPos; // Позиция на экране сейчас (без джиттера)
layout (location = 6) out vec4 outPrevPos; // Позиция на экране в прошлом кадре (без джиттера)

layout(set = 0, binding = 0) uniform SceneData {
	mat4 view;
	mat4 proj;
	mat4 viewproj;

	// Для TAA
	mat4 viewProjNonJittered; // Текущая чистая камера
	mat4 prevViewProj;        // Прошлая чистая камера

	// Направленный источник света
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;

	// Тени
	mat4 cascadeMatrices[4]; // Матрицы света для 4 каскадов
	vec4 cascadeSplits;      // Дистанции разделения каскадов упакованы в vec4 (x, y, z, w)
} scene;

layout(buffer_reference, std430) readonly buffer ModelMatricesRef {

	mat4 currentModel; // Текущая матрица трансформации меша
	mat4 prevModel;    // Прошлая матрица трансформации меша
};

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
layout( push_constant ) uniform constants{

	uint64_t matrixAddress;
	VertexBuffer vertexBuffer;

	uint colorTextureID;
	uint metallicRoughnessTextureID;
	uint normalTextureID;
	uint occlusionTextureID;

	vec4 baseColorFactor;
	vec4 materialFactors; // x: roughness, y: metallic, z: emissive, w: padding
} PushConstants;

void main()
{
	//load vertex data from device adress
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

	ModelMatricesRef modelBuffer = ModelMatricesRef(PushConstants.matrixAddress);
	mat4 currentModelMatrix = modelBuffer.currentModel;
	mat4 prevModelMatrix = modelBuffer.prevModel;

	vec4 worldPos = currentModelMatrix * vec4(v.position, 1.0f);
	outWorldPos = worldPos.xyz;
	gl_Position = scene.viewproj * worldPos;

	outCurrentPos = scene.viewProjNonJittered * worldPos;

	vec4 prevWorldPos = prevModelMatrix * vec4(v.position, 1.0f);
	outPrevPos = scene.prevViewProj * prevWorldPos;

	mat3 modelMat3 = mat3(currentModelMatrix);
	mat3 normalMatrix = mat3(
		normalize(modelMat3[0]),
		normalize(modelMat3[1]),
		normalize(modelMat3[2])
	);

	vec3 N = normalize(normalMatrix * v.normal);
	outNormal = N;

	vec3 T = normalize(normalMatrix * v.tangent.xyz);
	T = normalize(T - dot(T, N) * N);

	outTangent = vec4(T, v.tangent.w);

	// Для альбедо текстур
	outUV = vec2(v.uv_x, v.uv_y);
	outColor = v.color;
}