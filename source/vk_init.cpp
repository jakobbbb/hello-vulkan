#include "vk_init.h"

VkCommandPoolCreateInfo vkinit::command_pool_create_info(
    uint32_t queue_family,
    VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        // allow resetting individual command buffers
        .flags = flags,
        .queueFamilyIndex = queue_family,
    };
    return command_pool_info;
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(
    VkCommandPool pool,
    uint32_t count,
    VkCommandBufferLevel level) {
    VkCommandBufferAllocateInfo command_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = pool,
        .level = level,
        .commandBufferCount = count,
    };
    return command_buffer_info;
}
