#ifndef HELLO_ENGINE_H
#define HELLO_ENGINE_H

#include "engine.h"

// Rule of thumb:  Only vec4 and mat4
struct GPUSceneData {
    glm::vec4 fog_color;
    glm::vec4 fog_distances;  // min, max, unused, unused
    glm::vec4 ambient_color;
    glm::vec4 sun_direction;  // x, y, z, power
    glm::vec4 sun_color;
};

class HelloEngine : public Engine {
   public:
    // Scene stuff
    GPUSceneData _scene_data;
    AllocatedBuffer _scene_data_buf;

    // Descriptor layouts
    VkDescriptorSetLayout _global_set_layout;
    VkDescriptorSetLayout _obj_set_layout;

    Material _point_pipeline;

   protected:
    virtual void init_descriptors() override;
    virtual void init_pipelines() override;
    void init_pointcloud_pipeline();
    virtual void init_materials() override;
    virtual void init_scene() override;

    virtual void load_meshes() override;
    void update_meshes();

    /**
     * Upload mesh using a staging buffer.
     */
    void upload_mesh(Mesh& mesh, bool create_bufs = true);

    /**
     * Upload mesh using a HOST_VISIBLE and DEVICE_LOCAL buffer.
     */
    void upload_mesh_old(Mesh& mesh);

    virtual void render_pass(VkCommandBuffer cmd) override;
};

#endif  // HELLO_ENGINE_H
