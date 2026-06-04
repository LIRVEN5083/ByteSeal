#version 460

in vec3 NearP;
in vec3 FarP;

out vec4 frag_color;

uniform mat4 u_Projection;
uniform mat4 u_View;

vec4 grid(vec3 fragPos3D, float scale){
	//Размер единицы сетки
	vec2 coord = fragPos3D.xz * scale;
	//fwidth — это встроенная функция GLSL. Она вычисляет, насколько сильно меняется значение coord между соседними пикселями экрана.
	vec2 derivative = fwidth(coord);
	//Центрирование линий
	vec2 grid = abs(coord - floor(coord + 0.5)) / derivative;
	float line = min(grid.x, grid.y);

	float minimumz = min(derivative.y, 1.0);
    float minimumx = min(derivative.x, 1.0);

	vec4 color = vec4(0.2, 0.2, 0.2, 1.0); // Цвет линий сетки
    if (line < 1.0) {
        color = vec4(0.8, 0.8, 0.8, 1.0); // Яркость линий
    }
    return color;
}

void main(){
	float t = (- NearP.y / (FarP.y - NearP.y));
	if (t < 0.0) discard;
	vec3 fragPos3D = NearP + t * (FarP - NearP);

	vec4 projPos = u_Projection * u_View * vec4(fragPos3D, 1.0f);

	gl_FragDepth = (projPos.z / projPos.w) * 0.5 + 0.5;

	frag_color = grid(fragPos3D, 1.0f);
}