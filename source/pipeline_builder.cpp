#include "pipeline_builder.h"

#include <iostream>

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass) {
    // single viewport
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .viewportCount = 1,
        .pViewports = &_viewport,
        .scissorCount = 1,
        .pScissors = &_scissor,
    };

    // dummy color blending -- has to match frag shader outputs
    VkPipelineColorBlendStateCreateInfo blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &_color_blend_att,
    };

    // The Chantays - Pipeline (1963)
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .stageCount = (uint32_t)_stages.size(),
        .pStages = _stages.data(),
        .pVertexInputState = &_vert_input_info,
        .pInputAssemblyState = &_input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &_rasterizer,
        .pMultisampleState = &_multisampling,
        .pDepthStencilState = &_depth_stencil,
        .pColorBlendState = &blending,
        .layout = _layout,
        .renderPass = pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
    };

    VkPipeline pipeline;

    if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) !=
        VK_SUCCESS) {
        std::cerr << "Error creating gfx pipeline :(\n";
        return VK_NULL_HANDLE;  // :(
    } else {
        return pipeline;
    }
}
