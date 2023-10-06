#version 460

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;

layout (location = 0) out vec3 outColor;
layout (location = 1) out float outDist;

// Decriptor set at slot 0
// Binding 0 within that set
layout (set = 0, binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
} CameraData;

struct ObjectData {
    mat4 model_mat;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    ObjectData objects[];
} objectBuffer;

layout (push_constant) uniform constants {
    vec4 data;
    mat4 render_matrix;
} PushConstants;

void main() {
    mat4 model = objectBuffer.objects[gl_BaseInstance].model_mat;
    mat4 transform = CameraData.viewproj * model;
    gl_Position = transform * vec4(vPos, 1.0f);
    outColor = vColor;
    outDist = gl_Position.z / gl_Position.w;
    gl_PointSize = 3.f * pow(1.0f / outDist, 2);
}
