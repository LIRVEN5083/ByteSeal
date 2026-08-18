#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference : require
#extension GL_ARB_shader_viewport_layer_array : require

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;

    // Направленный источник света
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;

    // Твои каскады из первого сета (Set 0)
    mat4 cascadeMatrices[4]; 
    vec4 cascadeSplits;      
    uint shadowMapTextureID;
} scene;

struct Vertex {
    vec3 position; float uv_x;
    vec3 normal;   float uv_y;
    vec4 color;
    vec4 tangent;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform constants {
    mat4 worldMatrix;
    VertexBuffer vertexBuffer;
} PushConstants;

void main() {
    gl_Layer = gl_InstanceIndex; 

    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    // Считаем оригинальную позицию для X и Y
    vec4 pos = scene.cascadeMatrices[gl_InstanceIndex] * PushConstants.worldMatrix * vec4(v.position, 1.0);

    // Жестко заставляем W быть 1.0, а Z делаем 0.5 (ровно половина глубины)
    // Координаты X и Y делим на pos.w, чтобы объект не растянуло/не сжало из-за смены W
    gl_Position = vec4(pos.xy / pos.w, 0.5, 1.0);
}

