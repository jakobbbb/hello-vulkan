#ifndef ENGINE_H
#define ENGINE_H

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <iostream>
#include "vk_mesh.h"
#include "vk_types.h"

#define FRAME_OVERLAP 2

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

// Rule of thumb:  Only vec4 and mat4
struct GPUSceneData {
    glm::vec4 fog_color;
    glm::vec4 fog_distances;  // min, max, unused, unused
    glm::vec4 ambient_color;
    glm::vec4 sun_direction;  // x, y, z, power
    glm::vec4 sun_color;
};

struct GPUObjectData {
    glm::mat4 model_mat;
};

class Engine {
   public:
    VkPhysicalDeviceProperties _gpu_properties;

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

    // Renderpass
    VkRenderPass _render_pass;
    std::vector<VkFramebuffer> _framebuffers;

    // Sync
    FrameData _frames[FRAME_OVERLAP];

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

    std::vector<RenderObject> _scene;
    std::unordered_map<std::string, Material> _materials;
    std::unordered_map<std::string, Mesh> _meshes;

    // Descriptor stuff
    VkDescriptorSetLayout _global_set_layout;
    VkDescriptorSetLayout _obj_set_layout;
    VkDescriptorPool _descriptor_pool;

    // Scene
    GPUSceneData _scene_data;
    AllocatedBuffer _scene_data_buf;

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
    void init_descriptors();
    void init_pipelines();
    void init_scene();

    bool try_load_shader_module(const char* file_path, VkShaderModule* out);

    void load_meshes();
    void upload_mesh(Mesh& mesh);

    Material* create_mat(VkPipeline pipeline,
                         VkPipelineLayout layout,
                         std::string const& name);
    Material* get_mat(std::string const& name);
    Mesh* get_mesh(std::string const& name);
    void draw_objects(VkCommandBuffer cmd,
                      std::vector<RenderObject> const& scene);

    FrameData& get_current_frame();
    AllocatedBuffer create_buffer(size_t size,
                                  VkBufferUsageFlags usage,
                                  VmaMemoryUsage memory_usage);

    size_t pad_uniform_buf_size(size_t original_size) const;
};

#endif  // ENGINE_H
