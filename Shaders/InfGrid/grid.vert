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

float gGridSize = 1000.0;

layout(location = 0) out vec3 WorldPos;
layout(location = 1) noperspective out vec2 outCurrentPos;
layout(location = 2) noperspective out vec2 outPrevPos;

const vec3 Pos[4] = vec3[4](
    vec3(-1.0, -1.0, 0.0),      // bottom left
    vec3( 1.0, -1.0, 0.0),      // bottom right
    vec3( 1.0,  1.0, 0.0),      // top right
    vec3(-1.0,  1.0, 0.0)       // top left
);
const int Indices[6] = int[6](0, 2, 1, 2, 0, 3);

void main() {
    vec3 gCameraWorldPos = inverse(scene.view)[3].xyz;

    int Index = Indices[gl_VertexIndex];

    vec3 vPos3 = Pos[Index] * gGridSize;

    vPos3.x += gCameraWorldPos.x;
    vPos3.y += gCameraWorldPos.y;


    vec4 vPos4 = vec4(vPos3, 1.0);

    gl_Position = scene.viewproj * vPos4;

    vec4 clipCurrent = scene.viewProjNonJittered * vPos4;
    vec4 clipPrev = scene.prevViewProj * vPos4;

    outCurrentPos = clipCurrent.xy / clipCurrent.w;
    outPrevPos = clipPrev.xy / clipPrev.w;

    WorldPos = vPos3;
}

