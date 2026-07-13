#version 460

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj; // Если матрицы не доходят, проверим саму память
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} scene;

float gGridSize = 1000.0;

layout(location = 0) out vec3 WorldPos;

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

    WorldPos = vPos3;
}

