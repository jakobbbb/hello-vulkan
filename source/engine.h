#ifndef ENGINE_H
#define ENGINE_H

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

class Engine {
   public:
    bool _is_initialized{false};
    int _frame_number{0};

    VkExtent2D _window_extent{1280, 720};

    struct SDL_Window* _window{nullptr};

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
    bool load_shader_module(const char* file_path, VkShaderModule* out);
};

#endif  // ENGINE_H
