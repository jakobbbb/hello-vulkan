#ifndef VK_MESH_H
#define VK_MESH_H

#include <glm/vec3.hpp>
#include <vector>
#include "vk_types.h"

struct VertInputDesc {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attribs;
    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vert {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;

    static VertInputDesc get_desc();
};

struct Mesh {
    std::vector<Vert> verts;
    AllocatedBuffer buf;
};

Mesh make_simple_triangle();

#endif  // VK_MESH_H
