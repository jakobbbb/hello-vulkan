#ifndef VK_TYPES_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct AllocatedBuffer {
    VkBuffer _buf;
    VmaAllocation _alloc;
};

struct AllocatedImage {
    VkImage _img;
    VmaAllocation _alloc;
};

#define VK_TYPES_H
#endif  // VK_TYPES_H
