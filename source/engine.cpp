#include "engine.h"
#include <SDL.h>
#include <SDL_vulkan.h>
#include <iostream>

#ifndef APP_NAME
#define APP_NAME "Vulkan Engine"
#endif

#define VK_CHECK(x)                                       \
    do {                                                  \
        VkResult err = x;                                 \
        if (err) {                                        \
            std::cout << "Vulkan error: " << err << "\n"; \
            abort();                                      \
        }                                                 \
    } while (0)

void Engine::init() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)SDL_WINDOW_VULKAN;

    _window = SDL_CreateWindow(APP_NAME,
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               _window_extent.width,
                               _window_extent.height,
                               window_flags);

    _is_initialized = true;  // happy day
}

void Engine::cleanup() {
    if (_is_initialized) {
        SDL_DestroyWindow(_window);
    }
}

void Engine::draw() {
    // TODO
}

void Engine::run() {
    SDL_Event ev;
    bool run = true;

    while (run) {
        while (SDL_PollEvent(&ev) != 0) {
            if (ev.type == SDL_QUIT) {
                run = false;
            }
        }
        draw();
    }
}

void Engine::init_vulkan() {
    // Create Instance
    vkb::InstanceBuilder inst_builder;

    auto res = inst_builder.set_app_name(APP_NAME)
                   .request_validation_layers(true)
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
    vkb::Device dev = dev_builder.build().value();

    _device = dev.device;
    _phys_device = dev.physical_device;
}
