#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require

layout (location = 0) in vec4 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;
layout (location = 4) in vec4 inTangent;

layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform SceneData {
	mat4 view;
	mat4 proj;
	mat4 viewproj;

	// Направленный источник света
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;

	// Тени
	mat4 cascadeMatrices[4]; // Матрицы света для 4 каскадов
	vec4 cascadeSplits;      // Дистанции разделения каскадов упакованы в vec4 (x, y, z, w)
} scene;

layout(set = 1, binding = 0) uniform sampler2D globalTextures[];

// Для каскадов
layout(set = 1, binding = 1) uniform sampler2DArray globalTextureArray;

layout(set = 1, binding = 3, rgba16f) uniform readonly imageCube iblStorageMaps[];

struct Vertex {
	vec3 position; float uv_x;
	vec3 normal;   float uv_y;
	vec4 color;
	vec4 tangent;
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

// Усовершенствованный Френель
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Функция преобразования 3D-вектора луча (отражения или нормали) в 2D-пиксель + Face ID кубмапы.
// Она зеркально копирует логику GetCubeDirection из наших compute-шейдеров, 
// чтобы выборка из имидж-эррея imageCube[id] сошлась пиксель-в-пиксель.
/////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: IBL
ivec3 GetIBLFaceCoords(vec3 r, ivec2 size) {
    vec3 rotatedR = -vec3(r.y, r.z, r.x); 

    vec3 absR = abs(rotatedR);
    uint face = 0;
    vec2 uv = vec2(0.0);

    // Дальше идёт наша рабочая бесшовная математика
    if (absR.x >= absR.y && absR.x >= absR.z) {
        if (rotatedR.x > 0.0) { face = 0; uv = vec2(-rotatedR.z / absR.x,  rotatedR.y / absR.x); } // +X
        else                  { face = 1; uv = vec2( rotatedR.z / absR.x,  rotatedR.y / absR.x); } // -X
    } else if (absR.y >= absR.x && absR.y >= absR.z) {
        if (rotatedR.y > 0.0) { face = 2; uv = vec2( rotatedR.x / absR.y, -rotatedR.z / absR.y); } // +Y
        else                  { face = 3; uv = vec2( rotatedR.x / absR.y,  rotatedR.z / absR.y); } // -Y
    } else {
        if (rotatedR.z > 0.0) { face = 4; uv = vec2( rotatedR.x / absR.z,  rotatedR.y / absR.z); } // +Z
        else                  { face = 5; uv = vec2(-rotatedR.x / absR.z,  rotatedR.y / absR.z); } // -Z
    }

    uv = uv * 0.5 + 0.5;

    ivec2 pixelCoord = ivec2(uv * vec2(size));
    pixelCoord = clamp(pixelCoord, ivec2(0), size - ivec2(1));

    return ivec3(pixelCoord, face);
}

vec3 ACESFilm(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}




void main()
{
	// Получение Альбедо
	uint texID = nonuniformEXT(PushConstants.colorTextureID);
	vec4 texColor = texture(globalTextures[texID], inUV);
	vec4 finalAlbedo = inColor * texColor * PushConstants.baseColorFactor;

	if (finalAlbedo.a < 0.01f) {
		discard;
	}

	// Переводим альбедо из sRGB в Linear Space
	vec3 albedo = pow(finalAlbedo.rgb, vec3(2.2));

	float roughnessFactor = PushConstants.materialFactors.x;
	float metallicFactor  = PushConstants.materialFactors.y;

	float roughness;
	float metallic;

	// Проверяем: если ID равен 0 (или 0xFFFFFFFF, смотря что у тебя заглушка), текстуры нет
	if (PushConstants.metallicRoughnessTextureID == 0) {
		// Текстуры нет — используем чистые факторы из PushConstants как финальные значения
		roughness = roughnessFactor; 
		metallic  = metallicFactor;  
	} else {
		// Текстура есть — безопасно получаем динамический индекс внутри ветки else
		uint mrTexID = nonuniformEXT(PushConstants.metallicRoughnessTextureID);
		vec4 mrSample = texture(globalTextures[mrTexID], inUV);
		
		// Умножаем каналы текстуры на факторы (как требует стандарт glTF)
		roughness = mrSample.g * roughnessFactor;
		metallic  = mrSample.b * metallicFactor;
	}

// Защита от артефактов (слишком зеркальные поверхности могут ломать PBR блики)
roughness = max(roughness, 0.05f);

	// --ИНТЕГРАЦИЯ КАРТ НОРМАЛЕЙ--
	vec3 normal_vertex = normalize(inNormal);
	vec3 tangent_vertex = normalize(inTangent.xyz);
	
	tangent_vertex = normalize(tangent_vertex - dot(tangent_vertex, normal_vertex) * normal_vertex);
	vec3 bitangent_vertex = cross(normal_vertex, tangent_vertex) * inTangent.w;
	mat3 TBN = mat3(tangent_vertex, bitangent_vertex, normal_vertex);
	
	uint normTexID = nonuniformEXT(PushConstants.normalTextureID);
	vec3 localNormal = texture(globalTextures[normTexID], inUV).rgb;
	localNormal = localNormal * 2.0 - 1.0;
	
	vec3 N;
	if (PushConstants.normalTextureID == 0) {
		N = normalize(inNormal);
	} else {
		uint normTexID = nonuniformEXT(PushConstants.normalTextureID);
		vec3 localNormal = texture(globalTextures[normTexID], inUV).rgb;
		localNormal = normalize(localNormal * 2.0 - 1.0);
		N = normalize(TBN * localNormal); 
	}

	// ---------------------------

	// Вычисляем позицию камеры из матрицы view
	mat4 invView = inverse(scene.view);
	vec3 camPos = invView[3].xyz; 

	vec3 V = normalize(camPos - inWorldPos);
	vec3 L = normalize(scene.sunlightDirection.xyz);
	vec3 H = normalize(V + L);

	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);

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

	vec3 radiance = scene.sunlightColor.rgb * scene.sunlightDirection.w * 5.0;
	vec3 directLight = (kD * albedo / PI + specular) * radiance * NdotL;

	// --КАСКАДКИ НАХУЙ НАКОНЕЦ-ТО!--

	vec4 viewPos = scene.view * vec4(inWorldPos, 1.0);
	float depthValue = -viewPos.z;

	int cascadeIndex = 3;
	if (depthValue <= scene.cascadeSplits.x) cascadeIndex = 0;
	else if (depthValue <= scene.cascadeSplits.y) cascadeIndex = 1;
	else if (depthValue <= scene.cascadeSplits.z) cascadeIndex = 2;

	vec4 shadowCoord = scene.cascadeMatrices[cascadeIndex] * vec4(inWorldPos, 1.0);
	shadowCoord.xyz /= shadowCoord.w;

	vec3 uvw;
	uvw.xy = shadowCoord.xy * 0.5 + 0.5;
	uvw.z  = shadowCoord.z;

	float shadowTerm = 1.0; // 1.0 — полный свет, 0.0 — полная тень

	if (uvw.x >= 0.0 && uvw.x <= 1.0 && uvw.y >= 0.0 && uvw.y <= 1.0) {


		float bias = max(0.008 * (1.0 - dot(normal_vertex, L)), 0.005);
		if (cascadeIndex > 1) {
			bias *= 4.0;
		}

		vec2 shadowMapSize = textureSize(globalTextureArray, 0).xy;
		vec2 texelSize = 1.0 / shadowMapSize;

		float filterRadius = (cascadeIndex == 0) ? 2.5 : 1.5;

		float noise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
		float randomAngle = noise * 6.2831853;

		float c = cos(randomAngle);
		float s = sin(randomAngle);
		mat2 rotationMatrix = mat2(c, -s, s, c);

		vec2 poissonDisk[4] = vec2[](
			vec2(-0.7071,  0.7071), vec2( 0.7071,  0.7071),
			vec2(-0.7071, -0.7071), vec2( 0.7071, -0.7071)
		);

		float shadowSum = 0.0;

		for (int i = 0; i < 4; ++i) {
			vec2 offset = rotationMatrix * poissonDisk[i] * texelSize * filterRadius;
			vec2 sampleCoord = uvw.xy + offset;

			vec4 depths = textureGather(globalTextureArray, vec3(sampleCoord, float(cascadeIndex)), 0);


			vec4 shadowTests = step(depths - bias, vec4(uvw.z));

			vec2 f = fract(sampleCoord * shadowMapSize - 0.5);
			float shadowBottom = mix(shadowTests.x, shadowTests.y, f.x);
			float shadowTop = mix(shadowTests.w, shadowTests.z, f.x);
			float pcfSample = mix(shadowBottom, shadowTop, f.y);

			shadowSum += pcfSample;
		}

		shadowTerm = shadowSum / 4.0;
	}
	// -------------------------------------------------------------------------

	// Получение параметров Ambient Occlusion
	float ao = 1.0; 
	if (PushConstants.occlusionTextureID != 0) {
		uint occTexID = nonuniformEXT(PushConstants.occlusionTextureID);
		ao = texture(globalTextures[occTexID], inUV).r;
	}

	// Тень плавно гасит и диффузную, и спекулярную (бликовую) составляющую солнца
	vec3 finalDirectLight = directLight * mix(0.3, 1.0, ao) * shadowTerm;

	// --Image fucking based LIGHTING!--

	// Френель для IBL с учетом шероховатости поверхности
	vec3 F_IBL = fresnelSchlickRoughness(NdotV, F0, roughness);
	vec3 kS_IBL = F_IBL;
	vec3 kD_IBL = vec3(1.0) - kS_IBL;
	kD_IBL *= 1.0 - metallic;

	ivec2 irrSize = imageSize(iblStorageMaps[0]);
	ivec3 irrCoords = GetIBLFaceCoords(N, irrSize);
	vec3 irradiance = imageLoad(iblStorageMaps[0], irrCoords).rgb;
	vec3 iblDiffuse = irradiance * albedo;

	vec3 R = reflect(-V, N); 

	float rawMip = roughness * 4.0; // Значение от 0.0 до 4.0
	uint currentMipIdx = uint(floor(rawMip));
	uint nextMipIdx    = uint(ceil(rawMip));
	float mipInterpolation = fract(rawMip); 

	uint specBindlessIdx0 = 1 + currentMipIdx;
	uint specBindlessIdx1 = 1 + nextMipIdx;

	ivec2 specSize0 = imageSize(iblStorageMaps[specBindlessIdx0]);
	ivec3 specCoords0 = GetIBLFaceCoords(R, specSize0);
	vec3 prefilteredColor0 = imageLoad(iblStorageMaps[specBindlessIdx0], specCoords0).rgb;

	ivec2 specSize1 = imageSize(iblStorageMaps[specBindlessIdx1]);
	ivec3 specCoords1 = GetIBLFaceCoords(R, specSize1);
	vec3 prefilteredColor1 = imageLoad(iblStorageMaps[specBindlessIdx1], specCoords1).rgb;

	vec3 prefilteredColor = mix(prefilteredColor0, prefilteredColor1, mipInterpolation);
	

	ivec2 lutSize = imageSize(iblStorageMaps[6]);

	ivec2 lutPixelCoords = ivec2(NdotV * float(lutSize.x), (1.0 - roughness) * float(lutSize.y));
	lutPixelCoords = clamp(lutPixelCoords, ivec2(0), lutSize - ivec2(1));

	vec2 brdfSample = imageLoad(iblStorageMaps[6], ivec3(lutPixelCoords, 0)).rg;

	vec3 iblSpecular = prefilteredColor * (F_IBL * brdfSample.x + brdfSample.y);


	vec3 finalIBLDiffuse = iblDiffuse * (1.0 - metallic); 
	vec3 flatAmbient = scene.ambientColor.rgb * albedo * (1.0 - metallic);
	vec3 totalDiffuseAmbient = kD_IBL * (iblDiffuse + flatAmbient);

	vec3 iblAmbient = (totalDiffuseAmbient + iblSpecular) * ao;

	//-----------------------------------------------------------------------------
	// Финальный цвет (Свет + Тени)
	vec3 color = iblAmbient + finalDirectLight;

	// Шобы цвета подфиксить
	float exposure = 0.85; 
	color *= exposure;
	color = ACESFilm(color);
	color = pow(color, vec3(1.0 / 2.2));

	// --GLASS--
	float finalAlpha = finalAlbedo.a;

	if (finalAlbedo.a < 0.99f)
	{
		float specularIntensity = max(specular.r, max(specular.g, specular.b));
		float fresnelAlpha = pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);

		finalAlpha = max(finalAlpha, specularIntensity);
		finalAlpha = max(finalAlpha, fresnelAlpha * 0.5f); 
		finalAlpha = clamp(finalAlpha, 0.0f, 0.95f);
	}

	outFragColor = vec4(color, finalAlpha);
}