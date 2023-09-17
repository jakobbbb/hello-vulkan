#ifndef VK_MESH_H
#define VK_MESH_H

#include <glm/vec3.hpp>
#include <vector>
#include "vk_types.h"

struct Vert {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
};

struct Mesh {
    std::vector<Vert> _verts;
    AllocatedBuffer _buf;
};

Mesh make_simple_triangle();

#endif  // VK_MESH_H
