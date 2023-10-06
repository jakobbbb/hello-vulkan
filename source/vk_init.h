#ifndef VK_INIT_H
#define VK_INIT_H

#include <vulkan/vulkan.h>
#include "vk_mesh.h"

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
VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(
    VertInputDesc const& desc);

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

VkPipelineDepthStencilStateCreateInfo
depth_stencil_create_info(bool test, bool write, VkCompareOp compare);

VkDescriptorSetLayoutBinding descriptorset_layout_binding(
    VkDescriptorType type,
    VkShaderStageFlags stage_flags,
    uint32_t binding);

VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type,
                                             VkDescriptorSet dst_set,
                                             VkDescriptorBufferInfo* buf_info,
                                             uint32_t binding);

VkCommandBufferBeginInfo command_buffer_begin_info(
    VkCommandBufferUsageFlags flags = 0);

VkSubmitInfo submit_info(VkCommandBuffer* cmd);

}  // namespace vkinit

#endif  // VK_INIT_H
