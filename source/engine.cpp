#include "engine.h"
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
#define PRINT_DRAW_TIME

void Engine::init() {
    std::cout << "Initializing GLFW...\n";
    init_glfw();

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
    init_materials();

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
    glfwDestroyWindow(_window);
    glfwTerminate();

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

    VkClearValue clear = {
        .color = {1.0f, 1.0f, 1.0f, 1.0f},
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

    vkCmdBeginRenderPass(f.cmd, &rp_info, VK_SUBPASS_CONTENTS_INLINE);
    render_pass(f.cmd);
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
    bool run = true;

    while (run) {
        /* TODO adapt for glfw
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
        } */

        run = !glfwWindowShouldClose(_window);

#ifdef PRINT_DRAW_TIME
        auto start = std::chrono::high_resolution_clock::now();
#endif

        draw();  // :o

#ifdef PRINT_DRAW_TIME
        auto end = std::chrono::high_resolution_clock::now();
        auto us =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        auto ms = us.count() / 1000.f;
        _draw_times[_frame_number % _draw_times.size()] = ms;
#endif
    }
}

void Engine::init_glfw() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // non-OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);    // no resizing for now
    _window = glfwCreateWindow(_window_extent.width,
                               _window_extent.height,
                               APP_NAME,
                               nullptr,
                               nullptr);
}

void Engine::init_vulkan() {
    // Create Instance
    vkb::InstanceBuilder inst_builder;

    uint32_t glfw_ext_count = 0;
    auto glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);
    for (int i = 0; i < glfw_ext_count; ++i) {
        inst_builder.enable_extension(glfw_exts[i]);
    }

    auto res = inst_builder.set_app_name(APP_NAME)
                   .enable_validation_layers(true)
                   .require_api_version(1, 1, 0)
                   .use_default_debug_messenger()
                   .build();

    vkb::Instance inst = res.value();
    _instance = inst.instance;
    _debug_messenger = inst.debug_messenger;

    glfwCreateWindowSurface(_instance, _window, nullptr, &_surface);

    // Device
    vkb::PhysicalDeviceSelector selector{inst};
    vkb::PhysicalDevice phys_dev = selector.set_minimum_version(1, 1)
                                       .set_surface(_surface)
                                       .select()
                                       .value();

    vkb::DeviceBuilder dev_builder{phys_dev};
    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = nullptr,
    };
    features.features.fillModeNonSolid = VK_TRUE;
    VkPhysicalDeviceShaderDrawParametersFeatures features_draw_params = {
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
        .pNext = nullptr,
        .shaderDrawParameters = VK_TRUE,
    };
    vkb::Device dev = dev_builder.add_pNext(&features)
                          .add_pNext(&features_draw_params)
                          .build()
                          .value();

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
    _gpu_features = dev.physical_device.features;
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

VkViewport Engine::get_viewport() const {
    VkViewport viewport = {
        .x = 0.f,
        .y = 0.f,
        .width = (float)_window_extent.width,
        .height = (float)_window_extent.height,
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    return viewport;
}

VkRect2D Engine::get_scissor() const {
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = _window_extent,
    };
    return scissor;
}
