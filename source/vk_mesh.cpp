#include "vk_mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <iostream>

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

Vert Vert::from_idx(tinyobj::attrib_t const& attrib,
                    size_t vertex_idx,
                    size_t normal_idx) {
    // pos
    auto vx = attrib.vertices[vertex_idx + 0];
    auto vy = attrib.vertices[vertex_idx + 1];
    auto vz = attrib.vertices[vertex_idx + 2];
    // normal
    auto nx = attrib.normals[normal_idx + 0];
    auto ny = attrib.normals[normal_idx + 1];
    auto nz = attrib.normals[normal_idx + 2];
    Vert vert{
        .pos = {vx, vy, vz},
        .normal = {nx, ny, nz},
    };
    vert.color = vert.normal;
    return vert;
}

Mesh Mesh::make_simple_triangle() {
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

Mesh Mesh::make_point_cloud(size_t count) {
    std::vector<Vert> verts{};
    for (size_t i = 0; i < count; ++i) {
        glm::vec3 pos{
            (float)std::rand() / (float)RAND_MAX - 0.5f,
            (float)std::rand() / (float)RAND_MAX - 0.5f,
            (float)std::rand() / (float)RAND_MAX - 0.5f,
        };
        Vert v{
            .pos = pos,
            .color = pos + 0.5f,
        };
        verts.push_back(v);
    }
    return Mesh{.verts = verts};
}

Mesh Mesh::load_from_obj(const char* file_path, bool with_tris) {
    tinyobj::attrib_t attrib;  // vertex arrays
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    tinyobj::LoadObj(
        &attrib, &shapes, &materials, &warn, &err, file_path, ASSETS_DIRECTORY);

    if (!warn.empty()) {
        std::cerr << "Warning loading mesh '" << file_path << "': " << warn
                  << "\n";
    }
    if (!err.empty()) {
        std::cerr << "Error loading mesh '" << file_path << "': " << err
                  << "\n";
        return make_simple_triangle();
    }

    Mesh m{};
    size_t idx_offset = 0;
    for (auto shape : shapes) {
        if (with_tris) {  // as triangle list
            for (auto vert_count : shape.mesh.num_face_vertices) {
                int fv = 3;  // only tris for now
                for (size_t v = 0; v < fv; ++v) {
                    auto idx = shape.mesh.indices[idx_offset + v];
                    auto vert = Vert::from_idx(
                        attrib, fv * idx.vertex_index, fv * idx.normal_index);
                    m.verts.push_back(vert);
                }
                idx_offset += fv;
            }
        } else {  // just load verts
            auto n_verts = attrib.vertices.size() / 3;
            for (size_t i = 0; i < n_verts; ++i) {
                auto vert =
                    Vert::from_idx(attrib, 3 * i, 3 * i);  // TODO correct?
                m.verts.push_back(vert);
            }
        }
    }

    return m;
}
