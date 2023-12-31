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

VkPipelineShaderStageCreateInfo vkinit::pipeline_shader_stage_create_info(
    VkShaderStageFlagBits stage,
    VkShaderModule shader_module) {
    VkPipelineShaderStageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .stage = stage,
        .module = shader_module,
        .pName = "main",  // entry point
    };
    return info;
}

VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info() {
    VkPipelineVertexInputStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };
    return info;
}

VkPipelineVertexInputStateCreateInfo vkinit::vertex_input_state_create_info(
    VertInputDesc const& desc) {
    auto info = vkinit::vertex_input_state_create_info();
    info.vertexBindingDescriptionCount = desc.bindings.size();
    info.pVertexBindingDescriptions = desc.bindings.data();
    info.vertexAttributeDescriptionCount = desc.attribs.size();
    info.pVertexAttributeDescriptions = desc.attribs.data();
    return info;
}

VkPipelineInputAssemblyStateCreateInfo
vkinit::vertex_input_assembly_create_info(VkPrimitiveTopology topology) {
    VkPipelineInputAssemblyStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .topology = topology,  // tris, points, lines, ...
        .primitiveRestartEnable = VK_FALSE,
    };
    return info;
}

VkPipelineRasterizationStateCreateInfo vkinit::rasterization_state_create_info(
    VkPolygonMode polygon_mode) {
    VkPipelineRasterizationStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,  // would discard all primitives
        .polygonMode = polygon_mode,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp = 0.f,
        .depthBiasSlopeFactor = 0.f,
        .lineWidth = 1.f,
    };
    return info;
}

VkPipelineMultisampleStateCreateInfo vkinit::multisampling_state_create_info() {
    // no MSAA
    VkPipelineMultisampleStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    return info;
}

VkPipelineColorBlendAttachmentState vkinit::color_blend_attachment_state() {
    VkPipelineColorBlendAttachmentState att = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    return att;
}

VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info() {
    VkPipelineLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,  // no inputs (for now)
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 0,  // no inputs (for now)
        .pPushConstantRanges = nullptr,
    };
    return info;
}

VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        // allow to wait on fence before using it
        .flags = flags,
    };
    return info;
}

VkSemaphoreCreateInfo vkinit::semaphore_create_info(
    VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
    };
    return info;
}

VkBufferCreateInfo vkinit::buffer_create_info(size_t size,
                                              VkBufferUsageFlags usage) {
    VkBufferCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = size,
        .usage = usage,
    };
    return info;
}
VkImageCreateInfo vkinit::image_create_info(VkFormat format,
                                            VkImageUsageFlags flags,
                                            VkExtent3D extent) {
    VkImageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,  // how image is in gpu memory.
                                            // linear would allow cpu to r/w,
                                            // but it's slower
        .usage = flags,
    };
    return info;
}

VkImageViewCreateInfo vkinit::imageview_create_info(VkFormat format,
                                                    VkImage image,
                                                    VkImageAspectFlags flags) {
    VkImageSubresourceRange sub_range = {
        .aspectMask = flags,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = sub_range,
    };
    return info;
}

VkPipelineDepthStencilStateCreateInfo
vkinit::depth_stencil_create_info(bool test, bool write, VkCompareOp compare) {
    VkPipelineDepthStencilStateCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .depthTestEnable = test ? VK_TRUE : VK_FALSE,
        .depthWriteEnable = write ? VK_TRUE : VK_FALSE,
        .depthCompareOp = test ? compare : VK_COMPARE_OP_ALWAYS,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,  // optional
        .maxDepthBounds = 1.0f,  // optional
    };
    return info;
}

VkDescriptorSetLayoutBinding vkinit::descriptorset_layout_binding(
    VkDescriptorType type,
    VkShaderStageFlags stage_flags,
    uint32_t binding) {
    VkDescriptorSetLayoutBinding b = {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = 1,
        .stageFlags = stage_flags,
    };
    return b;
}

VkWriteDescriptorSet vkinit::write_descriptor_buffer(
    VkDescriptorType type,
    VkDescriptorSet dst_set,
    VkDescriptorBufferInfo* buf_info,
    uint32_t binding) {
    VkWriteDescriptorSet set_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = dst_set,
        .dstBinding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = buf_info,
    };
    return set_write;
}

VkCommandBufferBeginInfo vkinit::command_buffer_begin_info(
    VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pInheritanceInfo = nullptr,
    };
    return info;
}

VkSubmitInfo vkinit::submit_info(VkCommandBuffer* cmd) {
    VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = cmd,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };
    return info;
}
