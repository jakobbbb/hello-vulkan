#ifndef PIPELINE_BUILDER_H
#define PIPELINE_BUILDER_H

#include <vulkan/vulkan.h>
#include <vector>

class PipelineBuilder {
    public:
     std::vector<VkPipelineShaderStageCreateInfo> _stages;
     VkPipelineVertexInputStateCreateInfo _vert_input_info;
     VkPipelineInputAssemblyStateCreateInfo _input_assembly;
     VkViewport _viewport;
     VkRect2D _scissor;
     VkPipelineRasterizationStateCreateInfo _rasterizer;
     VkPipelineColorBlendAttachmentState _color_blend_att;
     VkPipelineMultisampleStateCreateInfo _multisampling;
     VkPipelineLayout _layout;

     VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};

#endif  // PIPELINE_BUILDER_H
