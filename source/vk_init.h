#ifndef VK_INIT_H
#define VK_INIT_H

#include <vulkan/vulkan.h>

namespace vkinit {

VkCommandPoolCreateInfo command_pool_create_info(
    uint32_t queue_family,
    VkCommandPoolCreateFlags flags = 0);

VkCommandBufferAllocateInfo command_buffer_allocate_info(
    VkCommandPool pool,
    uint32_t count = 1,
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
    VkShaderStageFlagBits flags,
    VkShaderModule shader_module);

VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();

VkPipelineInputAssemblyStateCreateInfo vertex_input_assembly_create_info(
    VkPrimitiveTopology topology);

VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(
    VkPolygonMode polygonMode);

VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();

VkPipelineColorBlendAttachmentState color_blend_attachment_state();

VkPipelineLayoutCreateInfo pipeline_layout_create_info();

VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

VkBufferCreateInfo buffer_create_info(size_t size, VkBufferUsageFlags usage);

VkImageCreateInfo image_create_info(VkFormat format,
                                    VkImageUsageFlags flags,
                                    VkExtent3D extent);

VkImageViewCreateInfo imageview_create_info(VkFormat format,
                                            VkImage image,
                                            VkImageAspectFlags flags);

}  // namespace vkinit

#endif  // VK_INIT_H
