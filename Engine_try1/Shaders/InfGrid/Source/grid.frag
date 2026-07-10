#version 450

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} scene;

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;

layout(location = 0) out vec4 outColor;

vec4 grid(vec3 fragPos3D, float scale) {
    // ВАЖНО: Для Z-Up сетка строится по осям X и Y!
    vec2 coord = fragPos3D.xy * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(coord / derivative - round(coord / derivative));
    float line = min(grid.x, grid.y);
    float minimum = min(derivative.x, derivative.y);

    // Базовый цвет линий сетки (серый)
    vec4 color = vec4(0.4, 0.4, 0.4, 1.0 - min(line, 1.0));

    // Подсветка главных осей в пространстве Z-Up
    if (fragPos3D.x > -0.1 * minimum && fragPos3D.x < 0.1 * minimum)
        color.y = 1.0; // Ось Y — ЗЕЛЕНАЯ
        if (fragPos3D.y > -0.1 * minimum && fragPos3D.y < 0.1 * minimum)
            color.x = 1.0; // Ось X — КРАСНАЯ

            return color;
}

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = scene.proj * scene.view * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

void main() {
    // Магия Z-Up: Ищем пересечение с плоскостью Z = 0
    float t = -nearPoint.z / (farPoint.z - nearPoint.z);

    if (t < 0.0) discard;

    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    gl_FragDepth = computeDepth(fragPos3D);

    // Плавное затухание к горизонту, чтобы не рябило вдалеке
    float linearDepth = (2.0 * 0.1) / (100.0 + 0.1 - gl_FragDepth * (100.0 - 0.1));
    float fading = max(0.0, (0.5 - linearDepth));

    // Смешиваем крупную (шаг 1.0) и мелкую (шаг 10.0) сетку
    outColor = (grid(fragPos3D, 1.0) + grid(fragPos3D, 0.1)) * fading;
}
