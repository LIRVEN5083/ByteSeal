#version 460

layout(location = 0) in vec3 WorldPos;
layout(location = 0) out vec4 FragColor;

// Добавляем UBO сцены для получения матрицы камеры
layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} scene;

float gGridSize = 1000.0;
float gGridMinPixelsBetweenCells = 2.0;
float gGridCellSize = 0.025;
vec4 gGridColorThin = vec4(0.5, 0.5, 0.5, 1.0);
vec4 gGridColorThick = vec4(0.0, 0.0, 0.0, 1.0);

float log10(float x) {
    return log(x) / log(10.0);
}

float satf(float x) {
    return clamp(x, 0.0, 1.0);
}

vec2 satv(vec2 x) {
    return clamp(x, vec2(0.0), vec2(1.0));
}

// Исправлено: заменено v.z на v.y для vec2
float max2(vec2 v) {
    return max(v.x, v.y);
}

void main() {
    // Получаем позицию камеры из матрицы view прямо на GPU
    vec3 gCameraWorldPos = inverse(scene.view)[3].xyz;

    vec2 dvx = vec2(dFdx(WorldPos.x), dFdy(WorldPos.x));
    vec2 dvz = vec2(dFdx(WorldPos.y), dFdy(WorldPos.y));

    float lx = length(dvx);
    float lz = length(dvz);

    vec2 dudv = vec2(lx, lz);
    float l = length(dudv);

    float LOD = max(0.0, log10(l * gGridMinPixelsBetweenCells / gGridCellSize) + 1.0);

    float GridCellSizeLod0 = gGridCellSize * pow(10.0, floor(LOD));
    float GridCellSizeLod1 = GridCellSizeLod0 * 10.0;
    float GridCellSizeLod2 = GridCellSizeLod1 * 10.0;

    dudv *= 4.0;

    // Исправлен синтаксис vec2(...)
    vec2 mod_div_dudv = mod(WorldPos.xy, GridCellSizeLod0) / dudv;
    float Lod0a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));

    mod_div_dudv = mod(WorldPos.xy, GridCellSizeLod1) / dudv;
    float Lod1a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));

    mod_div_dudv = mod(WorldPos.xy, GridCellSizeLod2) / dudv;
    float Lod2a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));

    float LOD_fade = fract(LOD);
    vec4 Color;

    if (Lod2a > 0.0) {
        Color = gGridColorThick;
        Color.a *= Lod2a;
    } else {
        if (Lod1a > 0.0) {
            Color = mix(gGridColorThick, gGridColorThin, LOD_fade);
            Color.a *= Lod1a;
        } else {
            Color = gGridColorThin;
            Color.a *= (Lod0a * (1.0 - LOD_fade));
        }
    }

    float dist = length(WorldPos.xy - gCameraWorldPos.xy);
    float OpacityFalloff = pow(satf(1.0 - (dist / gGridSize)), 2.0);
    Color.a *= OpacityFalloff;

    // Хак оптимизации: отбрасываем полностью прозрачные пиксели, чтобы не забивать филлрейт
    if (Color.a < 0.01) {
        discard;
    }

    FragColor = Color;
}
