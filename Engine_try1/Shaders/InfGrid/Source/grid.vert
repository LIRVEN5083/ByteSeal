#version 460

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
} scene;

layout(location = 0) out vec4 v_matrixRow;

const vec2 Pos[4] = vec2[4](
    vec2(-0.5, -0.5),
                            vec2( 0.5, -0.5),
                            vec2( 0.5,  0.5),
                            vec2(-0.5,  0.5)
);
const int Indices[6] = int[6](0, 2, 1, 2, 0, 3);

void main() {
    int Index = Indices[gl_VertexIndex];

    // Передаем первую строку (или колонку) матрицы viewproj во фрагментный шейдер
    // scene.viewproj[0] содержит компоненты (m00, m01, m02, m03)
    v_matrixRow = scene.viewproj[0];

    // Рисуем квадрат гарантированно по центру экрана, чтобы точно его видеть
    gl_Position = vec4(Pos[Index], 0.0, 1.0);
}
