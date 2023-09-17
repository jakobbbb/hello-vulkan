#ifndef VK_TYPES_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct AllocatedBuffer {
    VkBuffer buf;
    VmaAllocation alloc;
};

struct AllocatedImage {
    VkImage img;
    VmaAllocation alloc;
};

#define VK_TYPES_H
#endif  // VK_TYPES_H
