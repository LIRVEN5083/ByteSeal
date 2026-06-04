#version 460

layout (location = 0) in vec3 vertex_position;

uniform mat4 u_Projection;
uniform mat4 u_View;

out vec3 NearP;
out vec3 FarP;

vec3 UnprojectPoint(float x, float y, float z, mat4 viewInv, mat4 projInv) {
    vec4 unprojectedPoint = viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main(){
    mat4 viewInv = inverse(u_View);
    mat4 projInv = inverse(u_Projection);
    NearP = UnprojectPoint(vertex_position.x, vertex_position.y, 0.0, viewInv, projInv);
    FarP = UnprojectPoint(vertex_position.x, vertex_position.y, 1.0, viewInv, projInv);

    gl_Position = vec4 (vertex_position, 1.0);
}