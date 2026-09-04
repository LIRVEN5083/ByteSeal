#version 460

layout(location = 0) in vec3 inWorldViewDir;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outVelocity;
layout(location = 2) out vec4 outNormal;

layout(set = 1, binding = 0) uniform sampler2D globalTextures[];
// layout(set = 1, binding = 1) uniform sampler2DArray shadowCascades;
layout(set = 1, binding = 2) uniform sampler2D skyPanoramaTex; 

const vec2 invAtan = vec2(0.15915494, 0.31830988); // 1.0 / (2.0 * PI) и 1.0 / PI

vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.y, v.x), asin(v.z));

    uv *= invAtan;
    uv += 0.5;

    uv.y = 1.0 - uv.y; 
    
    return uv;
}

void main() {
    vec3 direction = normalize(inWorldViewDir);
    
    vec2 uv = SampleSphericalMap(direction);

    vec3 texColor = texture(skyPanoramaTex, uv).rgb;
    
    float brightness = dot(texColor, vec3(0.2126, 0.7152, 0.0722));

    if (isnan(texColor.r) || isnan(texColor.g) || isnan(texColor.b) ||
        isinf(texColor.r) || isinf(texColor.g) || isinf(texColor.b) ||
        texColor.r < 0.0f  || texColor.g < 0.0f  || texColor.b < 0.0f  ||
        brightness < 0.01f)
    {
        texColor = vec3(10000.0f); 
    }

    outColor = vec4(texColor, 1.0);
    outVelocity = vec2(0.0); // Небо не движется
    outNormal = vec4(0.0);
}
