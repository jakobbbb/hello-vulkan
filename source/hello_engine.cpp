#define VMA_IMPLEMENTATION

#include "hello_engine.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "pipeline_builder.h"
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

void HelloEngine::init_pipelines() {
    // shaders
    try_load_shader_module(SHADER_DIRECTORY "triangle.frag.spv", &_tri_frag);
    try_load_shader_module(SHADER_DIRECTORY "triangle.vert.spv", &_tri_vert);
    try_load_shader_module(SHADER_DIRECTORY "default_lit.frag.spv",
                           &_tri_rgb_frag);
    try_load_shader_module(SHADER_DIRECTORY "tri_rgb.vert.spv", &_tri_rgb_vert);
    try_load_shader_module(SHADER_DIRECTORY "tri_mesh.vert.spv",
                           &_tri_mesh_vert);

    auto vert = vkinit::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_VERTEX_BIT, _tri_vert);
    auto frag = vkinit::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_FRAGMENT_BIT, _tri_frag);
    auto vert_rgb = vkinit::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_VERTEX_BIT, _tri_rgb_vert);
    auto frag_rgb = vkinit::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_FRAGMENT_BIT, _tri_rgb_frag);
    auto vert_mesh = vkinit::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_VERTEX_BIT, _tri_mesh_vert);

    VkDescriptorSetLayout descriptor_set_layouts[] = {
        _global_set_layout,
        _obj_set_layout,
    };
    // Layout (Tri)
    VkPipelineLayoutCreateInfo layout_info =
        vkinit::pipeline_layout_create_info();
    layout_info.setLayoutCount = 2;  // default_lit.frag uses sceneData,
                                     // so we need to add the set layout for
                                     // this pipeline too
    layout_info.pSetLayouts = descriptor_set_layouts;
    VK_CHECK(vkCreatePipelineLayout(
        _device, &layout_info, nullptr, &_tri_pipeline_layout));

    // Layout (Mesh)
    VkPipelineLayoutCreateInfo layout_info_mesh =
        vkinit::pipeline_layout_create_info();
    VkPushConstantRange push_constant = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(MeshPushConstants),
    };
    layout_info_mesh.pPushConstantRanges = &push_constant;
    layout_info_mesh.pushConstantRangeCount = 1;
    layout_info_mesh.setLayoutCount = 2;
    layout_info_mesh.pSetLayouts = descriptor_set_layouts;
    VK_CHECK(vkCreatePipelineLayout(
        _device, &layout_info_mesh, nullptr, &_mesh_pipeline_layout));

    // Pipeline
    PipelineBuilder builder;
    builder._depth_stencil = vkinit::depth_stencil_create_info(
        true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder._stages.push_back(vert);
    builder._stages.push_back(frag);
    builder._vert_input_info =
        vkinit::vertex_input_state_create_info();  // soon...
    builder._input_assembly = vkinit::vertex_input_assembly_create_info(
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder._viewport = get_viewport();
    builder._scissor = get_scissor();

    builder._rasterizer =
        vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

    builder._multisampling = vkinit::multisampling_state_create_info();

    builder._color_blend_att = vkinit::color_blend_attachment_state();

    builder._layout = _tri_pipeline_layout;

    _tri_pipeline = builder.build_pipeline(_device, _render_pass);

    builder._stages.clear();
    builder._stages.push_back(vert_rgb);
    builder._stages.push_back(frag_rgb);
    _tri_rgb_pipeline = builder.build_pipeline(_device, _render_pass);

    auto vert_desc = Vert::get_desc();
    builder._vert_input_info =
        vkinit::vertex_input_state_create_info(vert_desc);

    builder._stages.clear();
    builder._stages.push_back(vert_mesh);
    builder._stages.push_back(frag_rgb);

    builder._layout = _mesh_pipeline_layout;
    _mesh_pipeline = builder.build_pipeline(_device, _render_pass);

    // pipelines created, so we can delete the shader modules
    vkDestroyShaderModule(_device, _tri_frag, nullptr);
    vkDestroyShaderModule(_device, _tri_vert, nullptr);
    vkDestroyShaderModule(_device, _tri_rgb_frag, nullptr);
    vkDestroyShaderModule(_device, _tri_rgb_vert, nullptr);
    vkDestroyShaderModule(_device, _tri_mesh_vert, nullptr);

    ENQUEUE_DELETE(vkDestroyPipeline(_device, _tri_pipeline, nullptr));
    ENQUEUE_DELETE(vkDestroyPipeline(_device, _tri_rgb_pipeline, nullptr));
    ENQUEUE_DELETE(vkDestroyPipeline(_device, _mesh_pipeline, nullptr));
    ENQUEUE_DELETE(
        vkDestroyPipelineLayout(_device, _tri_pipeline_layout, nullptr));
    ENQUEUE_DELETE(
        vkDestroyPipelineLayout(_device, _mesh_pipeline_layout, nullptr));

    // Also init pipeline for point clouds
    init_pointcloud_pipeline();
}

void HelloEngine::init_materials() {
    create_mat(_mesh_pipeline, _mesh_pipeline_layout, "mesh");
    create_mat(
        _point_pipeline.pipeline, _point_pipeline.pipeline_layout, "points");
}

void HelloEngine::load_meshes() {
    auto tri_mesh = Mesh::make_simple_triangle();
    upload_mesh(tri_mesh);
    _meshes["tri"] = tri_mesh;
    auto monkey_mesh = Mesh::make_point_cloud(1e6);
    upload_mesh(monkey_mesh);  // ~12ms/83.5fps
    _meshes["monkey"] = monkey_mesh;
    std::cout << "'Monkey' mesh has " << monkey_mesh.verts.size() / 1e6
              << "M verts, buffer is "
              << monkey_mesh.buf->alloc->GetSize() / 1e6 << "MB).\n";
}

void HelloEngine::update_meshes() {
    auto new_verts =
        Mesh::make_point_cloud(_meshes["monkey"].verts.size()).verts;
    _meshes["monkey"].verts = new_verts;
    upload_mesh(_meshes["monkey"], false);  // ~12ms/83.5fps
}

void HelloEngine::init_pointcloud_pipeline() {
    // Shaders
    VkShaderModule vert;
    try_load_shader_module(SHADER_DIRECTORY "point.vert.spv", &vert);
    auto vert_info = vkinit::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_VERTEX_BIT, vert);

    VkShaderModule frag;
    try_load_shader_module(SHADER_DIRECTORY "point.frag.spv", &frag);
    auto frag_info = vkinit::pipeline_shader_stage_create_info(
        VK_SHADER_STAGE_FRAGMENT_BIT, frag);

    // Descriptor sets
    VkDescriptorSetLayout descriptor_set_layouts[] = {
        _global_set_layout,
        _obj_set_layout,
    };

    // Layout
    auto layout_info = vkinit::pipeline_layout_create_info();
    layout_info.setLayoutCount = 2;
    layout_info.pSetLayouts = descriptor_set_layouts;
    VK_CHECK(vkCreatePipelineLayout(
        _device, &layout_info, nullptr, &_point_pipeline.pipeline_layout));
    ENQUEUE_DELETE(vkDestroyPipelineLayout(
        _device, _point_pipeline.pipeline_layout, nullptr));

    // build pipeline itself
    auto vert_desc = Vert::get_desc();
    PipelineBuilder builder = {
        ._stages = {vert_info, frag_info},
        ._vert_input_info = vkinit::vertex_input_state_create_info(vert_desc),
        ._input_assembly = vkinit::vertex_input_assembly_create_info(
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST),
        ._viewport = get_viewport(),
        ._scissor = get_scissor(),
        ._rasterizer =
            vkinit::rasterization_state_create_info(VK_POLYGON_MODE_POINT),
        ._color_blend_att = vkinit::color_blend_attachment_state(),
        ._multisampling = vkinit::multisampling_state_create_info(),
        ._layout = _point_pipeline.pipeline_layout,
        ._depth_stencil = vkinit::depth_stencil_create_info(
            true, true, VK_COMPARE_OP_LESS_OR_EQUAL),
    };
    _point_pipeline.pipeline = builder.build_pipeline(_device, _render_pass);
    ENQUEUE_DELETE(
        vkDestroyPipeline(_device, _point_pipeline.pipeline, nullptr));

    // shader modules can be destroted immedeately
    vkDestroyShaderModule(_device, frag, nullptr);
    vkDestroyShaderModule(_device, vert, nullptr);
}

void HelloEngine::upload_mesh(Mesh& mesh, bool create_bufs) {
    // Create staging buffer
    const size_t buf_size = mesh.verts.size() * sizeof(Vert);

    if (create_bufs) {
        VkBufferCreateInfo staging_buf_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .size = buf_size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        };

        VmaAllocationCreateInfo staging_alloc_info = {
            .usage = VMA_MEMORY_USAGE_CPU_ONLY,
        };

        mesh.buf = std::make_shared<AllocatedBuffer>();
        mesh.staging_buf = std::make_shared<AllocatedBuffer>();

        VK_CHECK(vmaCreateBuffer(_allocator,
                                 &staging_buf_info,
                                 &staging_alloc_info,
                                 &mesh.staging_buf->buf,
                                 &mesh.staging_buf->alloc,
                                 nullptr));

        // create and allocate vertex buffer on gpu
        VkBufferCreateInfo vertex_buf_info{staging_buf_info};
        vertex_buf_info.usage =
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VmaAllocationCreateInfo vertex_alloc_info = {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        };
        VK_CHECK(vmaCreateBuffer(_allocator,
                                 &vertex_buf_info,
                                 &vertex_alloc_info,
                                 &mesh.buf->buf,
                                 &mesh.buf->alloc,
                                 nullptr));
    }

    // copy vertex data to staging buffer
    void* data;
    vmaMapMemory(_allocator, mesh.staging_buf->alloc, &data);
    memcpy(data, mesh.verts.data(), mesh.verts.size() * sizeof(Vert));
    vmaUnmapMemory(_allocator, mesh.staging_buf->alloc);

    // copy to GPU
    immediate_submit([=](auto cmd) {
        VkBufferCopy copy = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = buf_size,
        };

        assert(nullptr != mesh.staging_buf->buf);
        assert(nullptr != mesh.buf->buf);

        vkCmdCopyBuffer(cmd,
                        mesh.staging_buf->buf,  // src
                        mesh.buf->buf,          // dst
                        1,                      // region count
                        &copy);
    });

    if (create_bufs) {
        // cleanup: staging buffer can be destroyed immedeately
        ENQUEUE_DELETE(vmaDestroyBuffer(
            _allocator, mesh.staging_buf->buf, mesh.staging_buf->alloc));
        ENQUEUE_DELETE(
            vmaDestroyBuffer(_allocator, mesh.buf->buf, mesh.buf->alloc));
    }
}

void HelloEngine::upload_mesh_old(Mesh& mesh) {
    auto buf_info = vkinit::buffer_create_info(
        mesh.verts.size() * sizeof(Vert), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    };

    // allocate
    VK_CHECK(vmaCreateBuffer(_allocator,
                             &buf_info,
                             &alloc_info,
                             &mesh.buf->buf,
                             &mesh.buf->alloc,
                             nullptr));
    ENQUEUE_DELETE(
        vmaDestroyBuffer(_allocator, mesh.buf->buf, mesh.buf->alloc));

    // upload vertex data
    void* data;
    vmaMapMemory(_allocator, mesh.buf->alloc, &data);
    memcpy(data, mesh.verts.data(), mesh.verts.size() * sizeof(Vert));
    vmaUnmapMemory(_allocator, mesh.buf->alloc);  // write finished, so unmap
}

void HelloEngine::render_pass(VkCommandBuffer cmd) {
    update_meshes();

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
