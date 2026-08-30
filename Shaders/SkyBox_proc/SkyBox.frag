#version 460

layout(location = 0) in vec3 outWorldViewDir;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
    mat4 cascadeMatrices[4]; // Твой чистый массив без лишних ID
    vec4 cascadeSplits;

    // Коэффициенты Хошека-Вилки
    vec4 skyA; vec4 skyB; vec4 skyC; vec4 skyD; vec4 skyE;
    vec4 skyF; vec4 skyG; vec4 skyH; vec4 skyI; vec4 skyZ; 
} scene;

float hosekWilkie(float cos_theta, float gamma, float cos_gamma, vec3 A, vec3 B, vec3 C, vec3 D, vec3 E, vec3 F, vec3 G, vec3 H, vec3 I) {
    float chi = (1.0 + cos_gamma * cos_gamma) / pow(1.0 + H.x * H.x - 2.0 * H.x * cos_gamma, 1.5);
    return (1.0 + A.x * exp(B.x / (cos_theta + 0.001))) * (C.x + D.x * exp(E.x * gamma) + F.x * (cos_gamma * cos_gamma) + G.x * chi + I.x * sqrt(max(0.0, cos_theta)));
}

vec3 getSkyColor(vec3 viewDir, vec3 sunDir, float cos_theta) {
    float cos_gamma = dot(viewDir, sunDir);       
    float gamma = acos(clamp(cos_gamma, -1.0, 1.0));

    vec3 skyColor;
    // Оригинальный чистый свиззлинг спектральных каналов Хошека
    skyColor.r = hosekWilkie(cos_theta, gamma, cos_gamma, scene.skyA.xyz, scene.skyB.xyz, scene.skyC.xyz, scene.skyD.xyz, scene.skyE.xyz, scene.skyF.xyz, scene.skyG.xyz, scene.skyH.xyz, scene.skyI.xyz) * scene.skyZ.x;
    skyColor.g = hosekWilkie(cos_theta, gamma, cos_gamma, scene.skyA.yzx, scene.skyB.yzx, scene.skyC.yzx, scene.skyD.yzx, scene.skyE.yzx, scene.skyF.yzx, scene.skyG.yzx, scene.skyH.yzx, scene.skyI.yzx) * scene.skyZ.y;
    skyColor.b = hosekWilkie(cos_theta, gamma, cos_gamma, scene.skyA.zxy, scene.skyB.zxy, scene.skyC.zxy, scene.skyD.zxy, scene.skyE.zxy, scene.skyF.zxy, scene.skyG.zxy, scene.skyH.zxy, scene.skyI.zxy) * scene.skyZ.z;

    return skyColor;
}

void main() {
    vec3 viewDir = normalize(outWorldViewDir);

    vec3 sunDir = normalize(scene.sunlightDirection.xyz);

    float cos_theta = clamp(abs(viewDir.z), 0.01, 1.0);

    float dayWeight = smoothstep(-0.02, 0.2, scene.sunlightDirection.z);

    if (scene.sunlightDirection.w <= 0.0) {
        dayWeight = 0.0;
    }

    float sunsetFactor = smoothstep(0.15, 0.0, clamp(scene.sunlightDirection.z, 0.0, 0.15));

    vec3 sunsetGlowColor = vec3(1.0, 0.45, 0.08);

    vec3 dayColor = getSkyColor(viewDir, sunDir, cos_theta);

    dayColor *= 1.2f;

    if (dayWeight > 0.0) {
        float cos_gamma = dot(viewDir, sunDir);
        float angleDistance = acos(clamp(cos_gamma, -1.0, 1.0));

        // Атмосферный ореол
        float haloIntensity = exp(-angleDistance * 4.0);

        vec3 currentHaloColor = mix(scene.sunlightColor.xyz, sunsetGlowColor, sunsetFactor * 0.8);
        float currentHaloPower = mix(0.15, 0.45, sunsetFactor); // Делаем закатное солнце визуально мощнее

        dayColor += currentHaloColor * haloIntensity * (scene.sunlightDirection.w * currentHaloPower);

        // Корона
        float coronaIntensity = exp(-angleDistance * 25.0);
        dayColor += scene.sunlightColor.xyz * coronaIntensity * (scene.sunlightDirection.w * 0.8);

        // Ядро диска солнца (рисуется строго в небе)
        if (viewDir.z > 0.0 && cos_gamma > 0.997) {
            float sunDisc = smoothstep(0.997, 0.9992, cos_gamma);

            vec3 coreColor = mix(scene.sunlightColor.xyz, vec3(1.0, 0.85, 0.5), sunsetFactor);
            dayColor += coreColor * sunDisc * scene.sunlightDirection.w * 5.0;
        }

        float horizonMask = pow(1.0 - cos_theta, 3.0);
        dayColor += sunsetGlowColor * horizonMask * (sunsetFactor * 0.35 * scene.sunlightDirection.w);
    }

    vec3 deepSpace = vec3(0.001, 0.0012, 0.002);
    float horizonFade = pow(1.0 - cos_theta, 4.0);
    vec3 nightHorizon = vec3(0.002, 0.003, 0.005);
    vec3 nightColor = mix(deepSpace, nightHorizon, horizonFade);

    vec3 skyColor = mix(nightColor, dayColor, dayWeight);

    float skyMask = step(0.0, viewDir.z);
    vec3 finalColor = skyColor * skyMask;

    // Тональное отображение Reinhard и гамма-коррекция 2.2
    finalColor = max(finalColor, vec3(0.0));
    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    outColor = vec4(finalColor, 1.0);
}
