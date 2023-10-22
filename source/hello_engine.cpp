#include "hello_engine.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "vk_init.h"
#include "vk_types.h"

void HelloEngine::init_descriptors() {
    // Layout
    auto cam_buf_binding = vkinit::descriptorset_layout_binding(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    auto scene_buf_binding = vkinit::descriptorset_layout_binding(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        1);
    VkDescriptorSetLayoutBinding bindings[] = {cam_buf_binding,
                                               scene_buf_binding};

    // descriptor set for global set
    VkDescriptorSetLayoutCreateInfo set_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 2,
        .pBindings = bindings,
    };
    VK_CHECK(vkCreateDescriptorSetLayout(
        _device, &set_info, nullptr, &_global_set_layout));
    ENQUEUE_DELETE(
        vkDestroyDescriptorSetLayout(_device, _global_set_layout, nullptr));

    // descriptor set for object storage buffer
    auto obj_buf_bind = vkinit::descriptorset_layout_binding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0);
    VkDescriptorSetLayoutCreateInfo obj_set_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &obj_buf_bind,
    };
    VK_CHECK(vkCreateDescriptorSetLayout(
        _device, &obj_set_info, nullptr, &_obj_set_layout));
    ENQUEUE_DELETE(
        vkDestroyDescriptorSetLayout(_device, _obj_set_layout, nullptr));

    // Uniform buffer for scene data
    // need one for each overlapping frame
    const size_t scene_data_buf_size =
        FRAME_OVERLAP * pad_uniform_buf_size(sizeof(GPUSceneData));
    _scene_data_buf = create_buffer(scene_data_buf_size,
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VMA_MEMORY_USAGE_CPU_TO_GPU);
    ENQUEUE_DELETE(vmaDestroyBuffer(
        _allocator, _scene_data_buf.buf, _scene_data_buf.alloc));

    // Pool holds 10 uniform buffers, 10 dynamic buffers, 10 storage buffers
    std::vector<VkDescriptorPoolSize> sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
    };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 10,
        .poolSizeCount = (uint32_t)sizes.size(),
        .pPoolSizes = sizes.data(),
    };
    VK_CHECK(vkCreateDescriptorPool(
        _device, &pool_info, nullptr, &_descriptor_pool));
    ENQUEUE_DELETE(vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr));

    // per-frame stuff
    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        // Buffers
        _frames[i].cam_buf = create_buffer(sizeof(GPUCameraData),
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VMA_MEMORY_USAGE_CPU_TO_GPU);
        ENQUEUE_DELETE(vmaDestroyBuffer(
            _allocator, _frames[i].cam_buf.buf, _frames[i].cam_buf.alloc));

        const int MAX_OBJECTS = 10000;
        _frames[i].obj_buf = create_buffer(sizeof(GPUObjectData) * MAX_OBJECTS,
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           VMA_MEMORY_USAGE_CPU_TO_GPU);
        ENQUEUE_DELETE(vmaDestroyBuffer(
            _allocator, _frames[i].obj_buf.buf, _frames[i].obj_buf.alloc));

        // Allocate Descriptor sets
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = _descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &_global_set_layout,
        };
        VK_CHECK(vkAllocateDescriptorSets(
            _device, &alloc_info, &_frames[i].global_descriptor));

        // point descriptor set 0 to camera data buffer
        VkDescriptorBufferInfo cam_buf_info = {
            .buffer = _frames[i].cam_buf.buf,
            .offset = 0,
            .range = sizeof(GPUCameraData),
        };
        auto cam_set_write =
            vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                            _frames[i].global_descriptor,
                                            &cam_buf_info,
                                            0);
        // descriptor set 1 is scene data buffer
        VkDescriptorBufferInfo scene_buf_info = {
            .buffer = _scene_data_buf.buf,
            .offset = 0,
            .range = sizeof(GPUSceneData),
        };
        auto scene_set_write = vkinit::write_descriptor_buffer(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            _frames[i].global_descriptor,
            &scene_buf_info,
            1);

        // object buffer
        VkDescriptorSetAllocateInfo obj_set_alloc = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = _descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &_obj_set_layout,
        };
        VK_CHECK(vkAllocateDescriptorSets(
            _device, &obj_set_alloc, &_frames[i].obj_descriptor));

        VkDescriptorBufferInfo obj_buf_info = {
            .buffer = _frames[i].obj_buf.buf,
            .offset = 0,
            .range = sizeof(GPUObjectData) * MAX_OBJECTS,
        };
        auto obj_set_write =
            vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            _frames[i].obj_descriptor,
                                            &obj_buf_info,
                                            0);

        // Finally, update descriptor sets
        VkWriteDescriptorSet write_descriptors[] = {
            cam_set_write,
            scene_set_write,
            obj_set_write,
        };
        vkUpdateDescriptorSets(_device,
                               3,  // descriptor write count
                               write_descriptors,
                               0,  // descriptor copy count
                               nullptr);
    }
}

void HelloEngine::render_pass(VkCommandBuffer cmd) {
    // camera
    glm::vec3 cam_pos = {
        0.f, 6.f * (0.95f + cos(_frame_number / 200.0f)), -10.f};
    auto view = glm::lookAt(cam_pos, glm::vec3{0, 4.f, 0}, glm::vec3{0, 1, 0});
    float aspect = (float)_window_extent.width / (float)_window_extent.height;
    glm::mat4 proj = glm::perspective(glm::radians(70.f), aspect, 0.1f, 200.f);
    proj[1][1] *= -1;
    GPUCameraData cam_data = {
        .view = view,
        .proj = proj,
        .viewproj = proj * view,
    };
    // copy to buffer
    void* p_cam_data;
    vmaMapMemory(_allocator, get_current_frame().cam_buf.alloc, &p_cam_data);
    memcpy(p_cam_data, &cam_data, sizeof(GPUCameraData));
    vmaUnmapMemory(_allocator, get_current_frame().cam_buf.alloc);

    // scene metadata
    _scene_data.ambient_color = {0.0f, 0.0f, 0.0f, 1};
    _scene_data.fog_color = {0.2f, 0.15f, 0.5f, 1.0f};
    _scene_data.fog_distances.x = 0.986f;
    _scene_data.fog_distances.y = 0.994f;

    // copy scene metadata
    char* p_scene_data;
    vmaMapMemory(_allocator, _scene_data_buf.alloc, (void**)&p_scene_data);
    p_scene_data += pad_uniform_buf_size(sizeof(GPUSceneData)) *
                    (_frame_number % FRAME_OVERLAP);
    memcpy(p_scene_data, &_scene_data, sizeof(GPUSceneData));
    vmaUnmapMemory(_allocator, _scene_data_buf.alloc);

    // object data
    void* p_obj_data;
    vmaMapMemory(_allocator, get_current_frame().obj_buf.alloc, &p_obj_data);
    GPUObjectData* objectSSBO = (GPUObjectData*)p_obj_data;
    for (int i = 0; i < _scene.size(); ++i) {
        auto obj = _scene[i];
        objectSSBO[i].model_mat = obj.transform;
    }
    vmaUnmapMemory(_allocator, get_current_frame().obj_buf.alloc);

    Mesh* last_mesh = nullptr;
    Material* last_mat = nullptr;
    int i = 0;
    for (auto obj : _scene) {
        if (obj.mat != last_mat) {
            last_mat = obj.mat;
            uint32_t scene_buf_offset =
                pad_uniform_buf_size(sizeof(GPUSceneData)) *
                (_frame_number % FRAME_OVERLAP);

            vkCmdBindPipeline(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, obj.mat->pipeline);
            vkCmdBindDescriptorSets(cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    obj.mat->pipeline_layout,
                                    0,  // first set
                                    1,  // descriptor set count
                                    &get_current_frame().global_descriptor,
                                    1,  // dynamic offsets
                                    &scene_buf_offset);
            vkCmdBindDescriptorSets(cmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    obj.mat->pipeline_layout,
                                    1,  // second set
                                    1,  // descriptor set count
                                    &get_current_frame().obj_descriptor,
                                    0,  // dynamic offsets
                                    nullptr);
        }

        MeshPushConstants push_constants = {
            .render_matrix = obj.transform,
        };

        vkCmdPushConstants(cmd,
                           _mesh_pipeline_layout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,
                           sizeof(MeshPushConstants),
                           &push_constants);

        if (obj.mesh != last_mesh) {
            last_mesh = obj.mesh;
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd,
                                   0,  // first binding
                                   1,  // binding count
                                   &(obj.mesh->buf->buf),
                                   &offset);
        }

        vkCmdDraw(cmd, obj.mesh->verts.size(), 1, 0, i++);
    }
}

void HelloEngine::init_scene() {
    RenderObject monkey = {
        .mesh = get_mesh("monkey"),
        .mat = get_mat("points"),
        .transform = glm::mat4{1.0f},
    };
    _scene.push_back(monkey);
    return;

    int radius = 40;
    for (int x = -radius; x <= radius; ++x) {
        for (int y = -radius; y <= radius; ++y) {
            if (sqrt(x * x + y * y) > radius) {
                continue;
            }
            glm::vec3 pos = {x, 0, y};
            auto translate = glm::translate(glm::mat4{1.0f}, pos);
            auto scale =
                glm::scale(glm::mat4{1.0f}, glm::vec3{0.5f, 0.5f, 0.5f});
            auto transform = translate * scale;
            auto look = glm::lookAt(-pos, glm::vec3(0.f), glm::vec3{0, 1, 0});
            RenderObject tri = {
                .mesh = get_mesh("monkey"),
                .mat = get_mat("mesh"),
                .transform = transform * look,
            };
            _scene.push_back(tri);
        }
    }
}
