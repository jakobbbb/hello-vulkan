#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in float inDist;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 1) uniform SceneData{
    vec4 fog_color;
    vec4 fog_distances;  // min, max, unused, unused
    vec4 ambient_color;
    vec4 sun_direction;  // x, y, z, power
    vec4 sun_color;
} sceneData;

// inverse lerp
float inv_mix(float x, float y, float a) {
    return (a - x) / (y - x);
}

void main() {
    vec3 fog = vec3(0, 0, 0);
    float fog_p = clamp(inv_mix(sceneData.fog_distances.x, sceneData.fog_distances.y, inDist), 0.0f, 1.0f);
    vec3 lit = inColor + sceneData.ambient_color.xyz;
    outFragColor = vec4(fog_p * sceneData.fog_color.rgb + (1 - fog_p) * lit, 1.0f);
}
