#include "vk_mesh.h"

Mesh make_simple_triangle() {
    return Mesh{.verts = {
                    {
                        .pos = {1, 1, 0},
                        .color = {1, 0, 0},
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
