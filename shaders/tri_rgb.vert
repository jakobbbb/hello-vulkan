#version 450

layout (location = 0) out vec3 outColor;
layout (location = 1) out float outDist;

void main() {
    const vec3 positions[3] = vec3[3](
        vec3(1.f,1.f, 0.0f),
        vec3(-1.f,1.f, 0.0f),
        vec3(0.f,-1.f, 0.0f)
    );

    // gaming
    const vec3 colors[3] = vec3[3](
        vec3(1.f, 0.f, 0.f),
        vec3(1.f, 1.f, 0.f),
        vec3(0.f, 0.f, 1.f)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 1.f);
    outColor = colors[gl_VertexIndex];
    outDist = 0.0f;  // no fog for triangles
}
