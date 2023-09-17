#include "vk_mesh.h"

VertInputDesc Vert::get_desc() {
    VkVertexInputBindingDescription main_binding = {
        .binding = 0,
        .stride = sizeof(Vert),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    VkVertexInputAttributeDescription pos_attr = {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,  // maps to glm::vec3
        .offset = offsetof(Vert, pos),
    };
    VkVertexInputAttributeDescription normal_attr = {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vert, normal),
    };
    VkVertexInputAttributeDescription color_attr = {
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vert, color),
    };

    return VertInputDesc{
        .bindings = {main_binding},
        .attribs = {pos_attr, normal_attr, color_attr},
    };
}

Mesh make_simple_triangle() {
    return Mesh{.verts = {
                    {
                        .pos = {1, 1, 0},
                        .color = {0, 0, 0.2},
                    },
                    {
                        .pos = {-1, 1, 0},
                        .color = {1, 0, 0},
                    },
                    {
                        .pos = {0, -1, 0},
                        .color = {1, 0, 0},
                    },
                }};
}
