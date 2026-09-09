#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_shader_viewport_layer_array : require
#extension GL_ARB_gpu_shader_int64 : require

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
	mat4 currentModel; // Текущая матрица модели
	mat4 prevModel;    // Прошлая матрица модели (в тенях не нужна, но в структуре лежит)
};

struct Vertex {
    vec3 position; float uv_x;
    vec3 normal;   float uv_y;
    vec4 color;
    vec4 tangent;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout( push_constant ) uniform constants{
	uint64_t matrixAddress;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() {
    gl_Layer = gl_InstanceIndex; 

    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    ModelMatricesRef modelBuffer = ModelMatricesRef(PushConstants.matrixAddress);

    mat4 currentModelMatrix = modelBuffer.currentModel;

    gl_Position = scene.cascadeMatrices[gl_InstanceIndex] * currentModelMatrix * vec4(v.position, 1.0);
}
