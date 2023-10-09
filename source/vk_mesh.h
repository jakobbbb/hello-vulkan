#ifndef VK_MESH_H
#define VK_MESH_H

#include <tiny_obj_loader.h>
#include <glm/vec3.hpp>
#include <memory>
#include <vector>
#include "vk_types.h"

#define ASSETS_DIRECTORY "../../assets/"

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
    static Vert from_idx(tinyobj::attrib_t const& attrib,
                         size_t vertex_idx,
                         size_t normal_idx);
};

struct Mesh {
    std::vector<Vert> verts;
    std::shared_ptr<AllocatedBuffer> buf;
    std::shared_ptr<AllocatedBuffer> staging_buf;

    static Mesh make_simple_triangle();
    static Mesh load_from_obj(const char* file_path, bool with_tris = true);
    static Mesh make_point_cloud(size_t count);
};

#endif  // VK_MESH_H
