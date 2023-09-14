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

}  // namespace vkinit

#endif  // VK_INIT_H
