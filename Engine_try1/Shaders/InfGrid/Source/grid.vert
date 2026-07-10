#version 450

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} scene;

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;

// ПРАВИЛЬНЫЙ СИНТАКСИС МАССИВА В GLSL:
// Тип с квадратными скобками vec3[] и явный вызов конструктора vec3[](...)
const vec3 gridPlane[4] = vec3[](
    vec3(-1.0, -1.0, 0.0),
                                 vec3( 1.0, -1.0, 0.0),
                                 vec3(-1.0,  1.0, 0.0),
                                 vec3( 1.0,  1.0, 0.0)
);

vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 proj) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(proj);
    vec4 unprojectedPoint = viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    // Достаем конкретную вершину по индексу из встроенной переменной Вулкана
    vec3 p = gridPlane[gl_VertexIndex];

    // Считаем точки луча для Z-Up пространства
    nearPoint = UnprojectPoint(p.x, p.y, 0.0, scene.view, scene.proj);
    farPoint = UnprojectPoint(p.x, p.y, 1.0, scene.view, scene.proj);

    gl_Position = vec4(p, 1.0);
}
