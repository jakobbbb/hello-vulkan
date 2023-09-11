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

     VkInstance _instance;
     VkDebugUtilsMessengerEXT _debug_messenger;
     VkPhysicalDevice _phys_device;
     VkDevice _device;
     VkSurfaceKHR _surface;

     void init();
     void cleanup();
     void draw();
     void run();

    private:
     void init_vulkan();
};

#endif  // ENGINE_H
