#version 460

layout(location = 0) in vec4 v_matrixRow; // Принимаем строку матрицы из вертексного
layout(location = 0) out vec4 FragColor;

void main() {
    // Если в матрице нули, v_matrixRow будет (0,0,0,0) -> цвет будет черный.
    // Если там есть числа (например, из-за вращения камеры),
    // abs() уберет минусы, и мы увидим яркие цвета (красный, зеленый, синий).
    FragColor = vec4(abs(v_matrixRow.xyz), 1.0);
}
