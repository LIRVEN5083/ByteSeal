#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform SceneData {
	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;
} scene;

layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

struct Vertex {
	vec3 position; float uv_x;
	vec3 normal;   float uv_y;
	vec4 color;
};
layout(buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};

layout( push_constant ) uniform constants
{
	mat4 worldMatrix;
	VertexBuffer vertexBuffer;

	uint colorTextureID;
	uint metallicRoughnessTextureID;
	uint normalTextureID;
	uint occlusionTextureID;

	vec2 padding;

	vec4 baseColorFactor;
	vec4 materialFactors;
} PushConstants;

const float PI = 3.14159265359;

// --- PBR ФУНКЦИИ ---

// Распределение микрограней (Trowbridge-Reitz GGX)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / max(denom, 0.000001);
}

// Геометрическое затенение (Geometry Schick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / max(denom, 0.000001);
}

// Геометрическая функция Смита (Smith's method)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

// Уравнение Френеля (Fresnel-Schlick)
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
	// Получение базового цвета (Альбедо)
	uint texID = nonuniformEXT(PushConstants.colorTextureID);
	vec4 texColor = texture(globalTextures[texID], inUV);
	vec4 finalAlbedo = inColor * texColor * PushConstants.baseColorFactor;

	if (finalAlbedo.a < 0.1f) {
		discard;
	}

	// Переводим альбедо из sRGB в Linear Space (если текстура загружена не как SRGB формат)
	vec3 albedo = pow(finalAlbedo.rgb, vec3(2.2));

	// Получение параметров материала
	float roughnessFactor = PushConstants.materialFactors.x;
	float metallicFactor  = PushConstants.materialFactors.y;

	uint mrTexID = nonuniformEXT(PushConstants.metallicRoughnessTextureID);
	vec4 mrSample = texture(globalTextures[mrTexID], inUV);

	// В glTF: roughness в Зеленом (G), metallic в Синем (B)
	float roughness = mrSample.g * roughnessFactor;
	float metallic  = mrSample.b * metallicFactor;

	// Ограничиваем roughness снизу, чтобы избежать деления на ноль при расчетах бликов
	roughness = max(roughness, 0.05);

	// Подготовка векторов (Z-up учитывается направлениями векторов в World Space)
	vec3 N = normalize(inNormal); // Пока без карты нормалей

	// Вычисляем позицию камеры из матрицы view (инвертируем вращение трансляции)
	mat3 rotMat = mat3(scene.view);
	vec3 d = vec3(scene.view[3]);
	vec3 camPos = -d * rotMat;

	vec3 V = normalize(camPos - inWorldPos);      // Вектор к камере
	vec3 L = -normalize(scene.sunlightDirection.xyz); // Вектор К солнцу (инвертируем входящий свет)
	vec3 H = normalize(V + L);                    // Вектор полупути (Half-vector)

	// Косинусы углов
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

	// F0 для диэлектриков в среднем 0.04. Для металлов интерполируем к базовому цвету
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// Расчет Cook-Torrance BRDF (Прямой свет)
	float NDF = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, L, roughness);
	vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

	vec3 numerator = NDF * G * F;
	float denominator = 4.0 * NdotV * NdotL;
	vec3 specular = numerator / max(denominator, 0.0001);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	vec3 radiance = scene.sunlightColor.rgb * scene.sunlightDirection.w;
	vec3 directLight = (kD * albedo / PI + specular) * radiance * NdotL;

	uint occTexID = nonuniformEXT(PushConstants.occlusionTextureID);
	float ao = texture(globalTextures[occTexID], inUV).r;

	vec3 ambient = scene.ambientColor.rgb * albedo * ao;

	vec3 color = ambient + directLight;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

	outFragColor = vec4(color, finalAlbedo.a);
}
