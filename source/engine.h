#ifndef ENGINE_H
#define ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include "vk_mesh.h"
#include "vk_types.h"

#ifndef FRAME_OVERLAP
#define FRAME_OVERLAP 2
#endif  // FRAME_OVERLAP

#define SHADER_DIRECTORY "../shaders/"

#define VK_CHECK(x)                                                       \
    do {                                                                  \
        VkResult err = x;                                                 \
        if (err) {                                                        \
            std::cout << "Vulkan error: " << string_VkResult(err) << " (" \
                      << err << ")\n";                                    \
            throw std::runtime_error("Vulkan error.");                    \
        }                                                                 \
    } while (0)
#define ENQUEUE_DELETE(x) _del_queue.push([=]() { x; })

struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void push(std::function<void()>&& f) { deletors.push_back(f); }

    void flush() {
        std::cout << "Deleting " << deletors.size() << " things...\n";
        while (!deletors.empty()) {
            (*deletors.rbegin())();
            deletors.pop_back();
        }
    }
};

struct Material {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
};

struct RenderObject {
    Mesh* mesh;
    Material* mat;
    glm::mat4 transform;
};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};

struct GPUCameraData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
};

struct FrameData {
    VkSemaphore present_semaphore;
    VkSemaphore render_semaphore;
    VkFence render_fence;

    VkCommandPool command_pool;
    VkCommandBuffer cmd;

    AllocatedBuffer cam_buf;
    VkDescriptorSet global_descriptor;

    AllocatedBuffer obj_buf;
    VkDescriptorSet obj_descriptor;
};

struct GPUObjectData {
    glm::mat4 model_mat;
};

struct UploadContext {
    VkFence upload_fence;
    VkCommandPool command_pool;
    VkCommandBuffer cmd;
};

class Engine {
   public:
    VkPhysicalDeviceProperties _gpu_properties;
    VkPhysicalDeviceFeatures _gpu_features;

    bool _is_initialized{false};
    int _frame_number{0};
    std::vector<float> _draw_times;

    int _selected_shader{0};

    VkExtent2D _window_extent{1280, 720};
    // struct SDL_Window* _window{nullptr};

    VmaAllocator _allocator;

    DeletionQueue _del_queue;

    // Vulkan Init
    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debug_messenger;
    VkPhysicalDevice _phys_device;
    VkDevice _device;
    VkSurfaceKHR _surface;

    // Swapchain
    VkSwapchainKHR _swapchain;
    VkFormat _swapchain_format;
    std::vector<VkImage> _swapchain_imgs;
    std::vector<VkImageView> _swapchain_views;

    // Depth bufer
    VkImageView _depth_view;
    AllocatedImage _depth_img;
    VkFormat _depth_format;

    // Command setup
    VkQueue _gfx_queue;
    uint32_t _gfx_queue_family;

    // Renderpass
    VkRenderPass _render_pass;
    std::vector<VkFramebuffer> _framebuffers;

    // Sync
    FrameData _frames[FRAME_OVERLAP];

    // Uploading to GPU
    UploadContext _upload_context;

    // Methods
    void init();
    void cleanup();
    void draw();
    void run();

   protected:
    /**
     * Initialization functions.
     * TODO: Add parameters where nescessary or mark virtual.
     */

    void init_glfw();
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_default_renderpass();
    void init_framebuffers();
    void init_sync_structures();
    virtual void init_descriptors() = 0;
    virtual void init_pipelines() = 0;
    virtual void init_materials() = 0;
    virtual void init_scene(){};

    /** Load a compiled shader from `file_path` into VkShaderModule `out`. */
    bool try_load_shader_module(const char* file_path, VkShaderModule* out);

    virtual void load_meshes() = 0;

    /**
     * Called during the render pass.  Place draw commands etc. here.
     */
    virtual void render_pass(VkCommandBuffer cmd) = 0;

    /**
     * Get the current frame, out of the frames in flight.
     */
    FrameData& get_current_frame();

    AllocatedBuffer create_buffer(size_t size,
                                  VkBufferUsageFlags usage,
                                  VmaMemoryUsage memory_usage);

    /**
     * Pad `original_size` in accordance with minUniformBufferOffsetAlignment.
     */
    size_t pad_uniform_buf_size(size_t original_size) const;
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& fun);

    // Helpers for pipeline init
    VkViewport get_viewport() const;
    VkRect2D get_scissor() const;
};

#endif  // ENGINE_H
