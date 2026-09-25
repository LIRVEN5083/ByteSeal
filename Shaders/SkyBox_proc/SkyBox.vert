#version 460

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

layout(location = 0) out vec3 outWorldViewDir;

void main() {
    // Полноэкранный треугольник (3 вершины)
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = -1.0 + float((gl_VertexIndex & 2) << 1);
    

    gl_Position = vec4(x, y, 0.0, 1.0);

    mat4 invProj = inverse(scene.proj);
    vec4 viewSpacePos = invProj * vec4(x, y, 0.0, 1.0);
    vec3 viewSpaceDir = viewSpacePos.xyz / viewSpacePos.w;
    
    mat3 invViewRot = transpose(mat3(scene.view));

    outWorldViewDir = invViewRot * viewSpaceDir;
}
