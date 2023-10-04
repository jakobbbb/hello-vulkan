#version 450

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform SceneData{
    vec4 fog_color;
    vec4 fog_distances;  // min, max, unused, unused
    vec4 ambient_color;
    vec4 sun_direction;  // x, y, z, power
    vec4 sun_color;
} sceneData;

void main() {
    outFragColor = vec4(inColor + sceneData.ambient_color.xyz, 1.f);
}
