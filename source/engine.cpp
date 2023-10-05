#define VMA_IMPLEMENTATION
#include "engine.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <chrono>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <numeric>
#include "pipeline_builder.h"
#include "vk_init.h"
#include "vk_types.h"

#define APP_NAME "Vulkan Engine"
static constexpr uint64_t TIMEOUT_SECOND = 1000000000;  // ns
#define SHADER_DIRECTORY "../shaders/"
#define PRINT_DRAW_TIME

#define VK_CHECK(x)                                       \
    do {                                                  \
        VkResult err = x;                                 \
        if (err) {                                        \
            std::cout << "Vulkan error: " << err << "\n"; \
            throw std::runtime_error("Vulkan error.");    \
        }                                                 \
    } while (0)
#define ENQUEUE_DELETE(x) _del_queue.push([=]() { x; })

void Engine::init() {
    std::cout << "Initializing SDL...\n";
    init_sdl();

    std::cout << "Initializing Vulkan...\n";
    init_vulkan();

    std::cout << "Initializing Swapchain...\n";
    init_swapchain();

    std::cout << "Initializing Commands...\n";
    init_commands();

    std::cout << "Initializing Default Renderpass...\n";
    init_default_renderpass();

    std::cout << "Initializing Framebuffers...\n";
    init_framebuffers();

    std::cout << "Initializing Sync Structures...\n";
    init_sync_structures();

    std::cout << "Initializing Descriptors...\n";
    init_descriptors();

    std::cout << "Initializing Pipelines...\n";
    init_pipelines();

    std::cout << "Loading Meshes...\n";
    load_meshes();

    std::cout << "Initializing Scene...\n";
    init_scene();

#ifdef PRINT_DRAW_TIME
    _draw_times.resize(100);
#endif  // PRINT_DRAW_TIME

    _is_initialized = true;  // happy day
}

void Engine::cleanup() {
    if (!_is_initialized) {
        return;
    }

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        vkWaitForFences(
            _device, 1, &_frames[i].render_fence, true, 1 * TIMEOUT_SECOND);
    }
    _del_queue.flush();

    // vulkan stuff
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
    vkDestroyInstance(_instance, nullptr);

    // window
    SDL_DestroyWindow(_window);

#ifdef PRINT_DRAW_TIME
    std::cout << "Drew " << _frame_number << " frames.\n";
    auto total = std::reduce(_draw_times.cbegin(), _draw_times.cend());
    std::cout << "Average draw time over the last " << _draw_times.size()
              << " frames: " << total / _draw_times.size() << " ms ("
              << _draw_times.size() / (total / 1000.f) << " fps).\n";
#endif  // PRINT_DRAW_TIME
}

void Engine::draw() {
    auto f = get_current_frame();
    VK_CHECK(
        vkWaitForFences(_device, 1, &f.render_fence, true, 1 * TIMEOUT_SECOND));
    VK_CHECK(vkResetFences(_device, 1, &f.render_fence));

    // request image
    uint32_t swapchain_im_idx;
    VK_CHECK(vkAcquireNextImageKHR(_device,
                                   _swapchain,
                                   UINT64_MAX,  // anytime there are less than
                                                // ~5k objects in the scene,
                                                // any timeout other than this
                                                // (indefinite) causes a timeout
                                                // and subsequent crash
                                   f.present_semaphore,
                                   nullptr,
                                   &swapchain_im_idx));

    VK_CHECK(vkResetCommandBuffer(f.cmd, 0));
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        // buf will only be submitted once and rerecorded every frame
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,  // no secondary cmd bufs
    };
    // start recording
    VK_CHECK(vkBeginCommandBuffer(f.cmd, &begin_info));

    float flash = abs(sin(_frame_number / 120.f));
    VkClearValue clear = {
        .color =
            {
                _scene_data.fog_color.r,
                _scene_data.fog_color.g,
                _scene_data.fog_color.b,
                1.0f,
            },
    };

    VkClearValue depth_clear;
    depth_clear.depthStencil.depth = 1.f;

    VkClearValue clears[2] = {clear, depth_clear};

    VkRenderPassBeginInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = _render_pass,
        .framebuffer = _framebuffers[swapchain_im_idx],
        .clearValueCount = 2,
        .pClearValues = &clears[0],
    };
    rp_info.renderArea.offset.x = 0;
    rp_info.renderArea.offset.y = 0;
    rp_info.renderArea.extent = _window_extent;

    glm::mat4 spin = glm::rotate(
        glm::mat4{1.f}, glm::radians(_frame_number * 0.8f), glm::vec3(0, 1, 0));
    _scene[0].transform = glm::scale(
        glm::translate(spin, glm::vec3{0.f, 5.f, 0.f}), glm::vec3(5.f));

    vkCmdBeginRenderPass(f.cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
    draw_objects(f.cmd, _scene);
    vkCmdEndRenderPass(f.cmd);

    VK_CHECK(vkEndCommandBuffer(f.cmd));

    // submit to queue
    VkPipelineStageFlags wait_stage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        // wait on _present_semaphore
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &f.present_semaphore,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &f.cmd,
        // signal _render_semaphore
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &f.render_semaphore,
    };
    VK_CHECK(vkQueueSubmit(_gfx_queue, 1, &submit, f.render_fence));

    // presentation time
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &f.render_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &_swapchain,
        .pImageIndices = &swapchain_im_idx,
    };
    VK_CHECK(vkQueuePresentKHR(_gfx_queue, &present_info));

    ++_frame_number;
}

void Engine::run() {
    SDL_Event ev;
    bool run = true;

    while (run) {
        while (SDL_PollEvent(&ev) != 0) {
            if (ev.type == SDL_QUIT) {
                run = false;
            } else if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                    case (SDLK_SPACE):
                        _selected_shader ^= 1;
                        break;
                }
            }
        }
#ifdef PRINT_DRAW_TIME
        auto start = std::chrono::high_resolution_clock::now();
#endif
        draw();
#ifdef PRINT_DRAW_TIME
        auto end = std::chrono::high_resolution_clock::now();
        auto us =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        auto ms = us.count() / 1000.f;
        _draw_times[_frame_number % _draw_times.size()] = ms;
#endif
    }
}

void Engine::init_sdl() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)SDL_WINDOW_VULKAN;

    _window = SDL_CreateWindow(APP_NAME,
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               _window_extent.width,
                               _window_extent.height,
                               window_flags);
}

void Engine::init_vulkan() {
    // Create Instance
    vkb::InstanceBuilder inst_builder;

    auto res = inst_builder.set_app_name(APP_NAME)
                   .enable_validation_layers(true)
                   .require_api_version(1, 1, 0)
                   .use_default_debug_messenger()
                   .build();

    vkb::Instance inst = res.value();
    _instance = inst.instance;
    _debug_messenger = inst.debug_messenger;

    // Device
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    vkb::PhysicalDeviceSelector selector{inst};
    vkb::PhysicalDevice phys_dev = selector.set_minimum_version(1, 1)
                                       .set_surface(_surface)
                                       .select()
                                       .value();

    vkb::DeviceBuilder dev_builder{phys_dev};
    VkPhysicalDeviceShaderDrawParametersFeatures features = {
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
        .pNext = nullptr,
        .shaderDrawParameters = VK_TRUE,
    };
    vkb::Device dev = dev_builder.add_pNext(&features).build().value();

    _device = dev.device;
    _phys_device = dev.physical_device;

    // get graphics queue
    _gfx_queue = dev.get_queue(vkb::QueueType::graphics).value();
    _gfx_queue_family = dev.get_queue_index(vkb::QueueType::graphics).value();

    // allocator
    VmaAllocatorCreateInfo allocator_info = {
        .physicalDevice = _phys_device,
        .device = _device,
        .instance = _instance,
    };
    vmaCreateAllocator(&allocator_info, &_allocator);
    ENQUEUE_DELETE(vmaDestroyAllocator(_allocator));

    _gpu_properties = dev.physical_device.properties;
}

void Engine::init_swapchain() {
    vkb::SwapchainBuilder swapchain_builder{_phys_device, _device, _surface};
    vkb::Swapchain swapchain =
        swapchain_builder.use_default_format_selection()
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)  // hard vsync
            .set_desired_extent(_window_extent.width, _window_extent.height)
            .build()
            .value();
    _swapchain = swapchain.swapchain;
    _swapchain_imgs = swapchain.get_images().value();
    _swapchain_views = swapchain.get_image_views().value();
    _swapchain_format = swapchain.image_format;

    ENQUEUE_DELETE(vkDestroySwapchainKHR(_device, _swapchain, nullptr));

    // depth buffer
    VkExtent3D depth_ext = {
        _window_extent.width,
        _window_extent.height,
        1,
    };
    _depth_format = VK_FORMAT_D32_SFLOAT;
    auto depth_img_info = vkinit::image_create_info(
        _depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depth_ext);
    VmaAllocationCreateInfo depth_alloc_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        // ensure image is in GPU memory
        .requiredFlags =
            VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    vmaCreateImage(_allocator,
                   &depth_img_info,
                   &depth_alloc_info,
                   &_depth_img.img,
                   &_depth_img.alloc,
                   nullptr);
    auto depth_view_info = vkinit::imageview_create_info(
        _depth_format, _depth_img.img, VK_IMAGE_ASPECT_DEPTH_BIT);
    VK_CHECK(
        vkCreateImageView(_device, &depth_view_info, nullptr, &_depth_view));
    ENQUEUE_DELETE(vkDestroyImageView(_device, _depth_view, nullptr));
    ENQUEUE_DELETE(
        vmaDestroyImage(_allocator, _depth_img.img, _depth_img.alloc));
}

void Engine::init_commands() {
    // create command pool
    auto command_pool_info = vkinit::command_pool_create_info(
        _gfx_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        VK_CHECK(vkCreateCommandPool(
            _device, &command_pool_info, nullptr, &_frames[i].command_pool));

        // create command buffer
        auto command_buffer_info =
            vkinit::command_buffer_allocate_info(_frames[i].command_pool);
        VK_CHECK(vkAllocateCommandBuffers(
            _device, &command_buffer_info, &_frames[i].cmd));

        ENQUEUE_DELETE(
            vkDestroyCommandPool(_device, _frames[i].command_pool, nullptr));
    }

    // create upload command pool
    auto upload_command_pool_info =
        vkinit::command_pool_create_info(_gfx_queue_family);
    VK_CHECK(vkCreateCommandPool(_device,
                                 &upload_command_pool_info,
                                 nullptr,
                                 &_upload_context.command_pool));
    ENQUEUE_DELETE(
        vkDestroyCommandPool(_device, _upload_context.command_pool, nullptr));
    auto upload_command_info =
        vkinit::command_buffer_allocate_info(_upload_context.command_pool, 1);
    VK_CHECK(vkAllocateCommandBuffers(
        _device, &upload_command_info, &_upload_context.cmd));
}

void Engine::init_default_renderpass() {
    VkAttachmentDescription color_attachment = {
        // format needed by swapchain
        .format = _swapchain_format,
        // no MSAA
        .samples = VK_SAMPLE_COUNT_1_BIT,
        // clear when attachment is loaded
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        // keep attachment stored when render pass ends
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        // don't care about stencil
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        // don't care about stencil
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        // don't care
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        // want to present at end
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    // depth buffer
    VkAttachmentDescription depth_attachment = {
        .format = _depth_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = depth_attachment.finalLayout,
    };

    VkAttachmentDescription attachments[2] = {color_attachment,
                                              depth_attachment};

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        // hook depth attachment
        .pDepthStencilAttachment = &depth_attachment_ref,
    };

    VkSubpassDependency dep = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    VkSubpassDependency depth_dep = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };
    VkSubpassDependency deps[2] = {dep, depth_dep};

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 2,
        .pAttachments = &attachments[0],
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 2,
        .pDependencies = &deps[0],
    };

    VK_CHECK(
        vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass));

    ENQUEUE_DELETE(vkDestroyRenderPass(_device, _render_pass, nullptr));
}

void Engine::init_framebuffers() {
    VkFramebufferCreateInfo fb_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .renderPass = _render_pass,
        .attachmentCount = 1,
        .width = _window_extent.width,
        .height = _window_extent.height,
        .layers = 1,
    };

    const uint32_t swapchain_image_count = _swapchain_imgs.size();
    _framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);

    for (int i = 0; i < swapchain_image_count; ++i) {
        VkImageView attachments[2] = {_swapchain_views[i], _depth_view};
        fb_info.attachmentCount = 2;
        fb_info.pAttachments = &attachments[0];
        VK_CHECK(
            vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));

        ENQUEUE_DELETE({
            vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
            vkDestroyImageView(_device, _swapchain_views[i], nullptr);
        });
    }
}

void Engine::init_sync_structures() {
    // signeled bit: wait on this fence before sending commands
    auto fence_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    auto sema_info = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        VK_CHECK(vkCreateFence(
            _device, &fence_info, nullptr, &_frames[i].render_fence));
        ENQUEUE_DELETE(
            vkDestroyFence(_device, _frames[i].render_fence, nullptr));

        VK_CHECK(vkCreateSemaphore(
            _device, &sema_info, nullptr, &_frames[i].present_semaphore));
        VK_CHECK(vkCreateSemaphore(
            _device, &sema_info, nullptr, &_frames[i].render_semaphore));
        ENQUEUE_DELETE(
            vkDestroySemaphore(_device, _frames[i].present_semaphore, nullptr));
        ENQUEUE_DELETE(
            vkDestroySemaphore(_device, _frames[i].render_semaphore, nullptr));
    }

    // upload fence
    VkFenceCreateInfo upload_fence_info = vkinit::fence_create_info();
    VK_CHECK(vkCreateFence(
        _device, &upload_fence_info, nullptr, &_upload_context.upload_fence));
    ENQUEUE_DELETE(
        vkDestroyFence(_device, _upload_context.upload_fence, nullptr));
}

bool Engine::try_load_shader_module(const char* file_path,
                                    VkShaderModule* out) {
    std::ifstream file{file_path, std::ios::ate | std::ios::binary};

    if (!file.is_open()) {
        std::cerr << "Loading shader '" << file_path << "' failed: "
                  << "Could not open file\n ";
        return false;
    }

    // load into buffer
    size_t size = (size_t)file.tellg();
    std::vector<uint32_t> buf(size / sizeof(uint32_t));
    file.seekg(0);
    file.read((char*)buf.data(), size);
    file.close();

    // load into Vulkan
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .codeSize = buf.size() * sizeof(uint32_t),
        .pCode = buf.data(),
    };
    VkShaderModule shader_module;
    if (vkCreateShaderModule(_device, &create_info, nullptr, &shader_module) !=
        VK_SUCCESS) {
        std::cerr << "Loading shader '" << file_path << "' failed.";
        return false;
    }
    *out = shader_module;
    return true;
}

void Engine::init_descriptors() {
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

void Engine::init_pipelines() {
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

    builder._viewport.x = 0.f;
    builder._viewport.y = 0.f;
    builder._viewport.width = _window_extent.width;
    builder._viewport.height = _window_extent.height;
    builder._viewport.minDepth = 0.f;
    builder._viewport.maxDepth = 1.f;

    builder._scissor.offset = {0, 0};
    builder._scissor.extent = _window_extent;

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
    builder._vert_input_info.pVertexAttributeDescriptions =
        vert_desc.attribs.data();
    builder._vert_input_info.vertexAttributeDescriptionCount =
        vert_desc.attribs.size();
    builder._vert_input_info.pVertexBindingDescriptions =
        vert_desc.bindings.data();
    builder._vert_input_info.vertexBindingDescriptionCount =
        vert_desc.bindings.size();

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

    // create Materials
    create_mat(_mesh_pipeline, _mesh_pipeline_layout, "mesh");
}

void Engine::load_meshes() {
    auto tri_mesh = Mesh::make_simple_triangle();
    upload_mesh(tri_mesh);
    _meshes["tri"] = tri_mesh;
    auto monkey_mesh = Mesh::load_from_obj(ASSETS_DIRECTORY "monkey.obj");
    upload_mesh(monkey_mesh);
    _meshes["monkey"] = monkey_mesh;
}

void Engine::upload_mesh(Mesh& mesh) {
    // upload_mesh_old(mesh);
    // return;

    // Create staging buffer
    const size_t buf_size = mesh.verts.size() * sizeof(Vert);

    VkBufferCreateInfo staging_buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = buf_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };

    VmaAllocationCreateInfo staging_alloc_info = {
        .usage = VMA_MEMORY_USAGE_CPU_ONLY,
    };

    AllocatedBuffer staging_buf;
    VK_CHECK(vmaCreateBuffer(_allocator,
                             &staging_buf_info,
                             &staging_alloc_info,
                             &staging_buf.buf,
                             &staging_buf.alloc,
                             nullptr));

    // copy vertex data to staging buffer
    void* data;
    vmaMapMemory(_allocator, staging_buf.alloc, &data);
    memcpy(data, mesh.verts.data(), mesh.verts.size() * sizeof(Vert));
    vmaUnmapMemory(_allocator, staging_buf.alloc);

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
                             &mesh.buf.buf,
                             &mesh.buf.alloc,
                             nullptr));

    // copy to GPU
    immediate_submit([=](auto cmd) {
        VkBufferCopy copy = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = buf_size,
        };

        assert(nullptr != staging_buf.buf);
        assert(nullptr != mesh.buf.buf);

        vkCmdCopyBuffer(cmd,
                        staging_buf.buf,  // src
                        mesh.buf.buf,     // dst
                        1,                // region count
                        &copy);
    });

    // cleanup: staging buffer can be destroyed immedeately
    vmaDestroyBuffer(_allocator, staging_buf.buf, staging_buf.alloc);
    ENQUEUE_DELETE(vmaDestroyBuffer(_allocator, mesh.buf.buf, mesh.buf.alloc));
}

void Engine::upload_mesh_old(Mesh& mesh) {
    auto buf_info = vkinit::buffer_create_info(
        mesh.verts.size() * sizeof(Vert), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    };

    // allocate
    VK_CHECK(vmaCreateBuffer(_allocator,
                             &buf_info,
                             &alloc_info,
                             &mesh.buf.buf,
                             &mesh.buf.alloc,
                             nullptr));
    ENQUEUE_DELETE(vmaDestroyBuffer(_allocator, mesh.buf.buf, mesh.buf.alloc));

    // upload vertex data
    void* data;
    vmaMapMemory(_allocator, mesh.buf.alloc, &data);
    memcpy(data, mesh.verts.data(), mesh.verts.size() * sizeof(Vert));
    vmaUnmapMemory(_allocator, mesh.buf.alloc);  // write finished, so unmap
}

Material* Engine::create_mat(VkPipeline pipeline,
                             VkPipelineLayout layout,
                             std::string const& name) {
    Material mat = {
        .pipeline = pipeline,
        .pipeline_layout = layout,
    };
    _materials[name] = mat;
    return &_materials[name];
}

Material* Engine::get_mat(std::string const& name) {
    auto it = _materials.find(name);
    assert(it != _materials.end());
    auto p = &((*it).second);
    assert(p != nullptr);
    return p;
}

Mesh* Engine::get_mesh(std::string const& name) {
    auto it = _meshes.find(name);
    assert(it != _meshes.end());
    auto p = &((*it).second);
    assert(p != nullptr);
    return p;
}

void Engine::draw_objects(VkCommandBuffer cmd,
                          std::vector<RenderObject> const& scene) {
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
    _scene_data.ambient_color = {0.1f, 0.0f, 0.4f, 1};
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
    for (int i = 0; i < scene.size(); ++i) {
        auto obj = scene[i];
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
                                   &(obj.mesh->buf.buf),
                                   &offset);
        }

        vkCmdDraw(cmd, obj.mesh->verts.size(), 1, 0, i++);
    }
}

void Engine::init_scene() {
    RenderObject monkey = {
        .mesh = get_mesh("monkey"),
        .mat = get_mat("mesh"),
        .transform = glm::mat4{1.0f},
    };
    _scene.push_back(monkey);

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

FrameData& Engine::get_current_frame() {
    return _frames[_frame_number % FRAME_OVERLAP];
}

AllocatedBuffer Engine::create_buffer(size_t size,
                                      VkBufferUsageFlags usage,
                                      VmaMemoryUsage memory_usage) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = size,
        .usage = usage,
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = memory_usage,
    };

    AllocatedBuffer buf;

    VK_CHECK(vmaCreateBuffer(
        _allocator, &info, &alloc_info, &buf.buf, &buf.alloc, nullptr));
    return buf;
}

size_t Engine::pad_uniform_buf_size(size_t original_size) const {
    size_t min_alignment =
        _gpu_properties.limits.minUniformBufferOffsetAlignment;
    size_t aligned_size = original_size;
    if (min_alignment > 0) {
        aligned_size =
            (aligned_size + min_alignment - 1) & ~(min_alignment - 1);
    }
    return aligned_size;
}

void Engine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& fun) {
    VkCommandBuffer cmd = _upload_context.cmd;

    // flag: buffer will be used once before resetting
    auto cmd_begin_info = vkinit::command_buffer_begin_info(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // record to cmd
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));
    fun(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    // submit to queue, execute, wait and reset
    auto submit = vkinit::submit_info(&cmd);
    // we could use a separate queue here, e.g. to load from a separate thread
    VK_CHECK(
        vkQueueSubmit(_gfx_queue, 1, &submit, _upload_context.upload_fence));
    vkWaitForFences(_device,
                    1,
                    &_upload_context.upload_fence,
                    VK_TRUE,
                    100 * TIMEOUT_SECOND);
    vkResetFences(_device, 1, &_upload_context.upload_fence);

    vkResetCommandPool(_device, _upload_context.command_pool, 0);
}
