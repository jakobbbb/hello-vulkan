#ifndef ENGINE_H
#define ENGINE_H

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include "vk_mesh.h"
#include "vk_types.h"

struct DeletionQueue {
    std::deque<std::function<void()>> deletors;

    void push(std::function<void()>&& f) { deletors.push_back(f); }

    void flush() {
        while (!deletors.empty()) {
            (*deletors.rbegin())();
            deletors.pop_back();
        }
    }
};

struct MeshPushConstants {
    glm::vec4 data;
    glm::mat4 render_matrix;
};

class Engine {
   public:
    bool _is_initialized{false};
    int _frame_number{0};

    int _selected_shader{0};

    VkExtent2D _window_extent{1280, 720};
    struct SDL_Window* _window{nullptr};

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
    VkCommandPool _command_pool;
    VkCommandBuffer _command_buf;

    // Renderpass
    VkRenderPass _render_pass;
    std::vector<VkFramebuffer> _framebuffers;

    // Sync structures
    VkSemaphore _present_semaphore;
    VkSemaphore _render_semaphore;
    VkFence _render_fence;

    // Shader modules
    VkShaderModule _tri_frag;
    VkShaderModule _tri_vert;
    VkShaderModule _tri_rgb_frag;
    VkShaderModule _tri_rgb_vert;
    VkShaderModule _tri_mesh_vert;

    // Pipeline stuff
    VkPipelineLayout _tri_pipeline_layout;
    VkPipelineLayout _mesh_pipeline_layout;
    VkPipeline _tri_pipeline;
    VkPipeline _tri_rgb_pipeline;
    VkPipeline _mesh_pipeline;

    Mesh _tri_mesh;
    Mesh _monkey_mesh;

    // Methods
    void init();
    void cleanup();
    void draw();
    void run();

   private:
    void init_sdl();
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_default_renderpass();
    void init_framebuffers();
    void init_sync_structures();
    void init_pipelines();

    bool try_load_shader_module(const char* file_path, VkShaderModule* out);

    void load_meshes();
    void upload_mesh(Mesh& mesh);
};

#endif  // ENGINE_H
