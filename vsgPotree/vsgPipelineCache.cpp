#include <vsg/all.h>
#include "vsgPipelineCache.h"

vsgPotree::PipelineCache::PipelineCache()
{
}

static const char* vertexShaderSource = R"(

#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants
{
    mat4 projection;
    mat4 view;
} pc;

void main()
{
    gl_Position = pc.projection * pc.view * vec4(inPosition, 1.0);

    fragColor = inColor;

    gl_PointSize = 3.0;
}

)";

static const char* fragmentShaderSource = R"(

#version 450

layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = fragColor;
}

)";

vsg::ref_ptr<vsg::BindGraphicsPipeline> vsgPotree::PipelineCache::getOrCreateBindGraphicsPipeline(uint32_t geometryAttributesMask, vsg::ref_ptr<const vsg::Options> options)
{
    // check to see if pipeline has already been created
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (auto itr = m_pipelineMap.find(geometryAttributesMask); itr != m_pipelineMap.end())
        {
            return itr->second;
        }
    }


    auto vertexShader = vsg::ShaderStage::create(
        VK_SHADER_STAGE_VERTEX_BIT,
        "main",
        vertexShaderSource);

    auto fragmentShader = vsg::ShaderStage::create(
        VK_SHADER_STAGE_FRAGMENT_BIT,
        "main",
        fragmentShaderSource);

    vsg::ShaderStages shaderStages{
        vertexShader,
        fragmentShader
    };

    vsg::VertexInputState::Bindings vertexBindingsDescriptions;
    vsg::VertexInputState::Attributes vertexAttributeDescriptions;
    {
        uint32_t vertexBindingIndex = 0;
        // setup vertex array
        {
            vertexBindingsDescriptions.push_back(VkVertexInputBindingDescription{ vertexBindingIndex, sizeof(vsg::vec3), VK_VERTEX_INPUT_RATE_VERTEX });
            vertexAttributeDescriptions.push_back(VkVertexInputAttributeDescription{VERTEX_CHANNEL, vertexBindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0 });
            vertexBindingIndex++;
        }

        if (geometryAttributesMask& COLOR)
        {
            VkVertexInputRate color_rate = VK_VERTEX_INPUT_RATE_VERTEX;
            vertexBindingsDescriptions.push_back(VkVertexInputBindingDescription{ vertexBindingIndex, sizeof(vsg::vec4), color_rate });
            vertexAttributeDescriptions.push_back(VkVertexInputAttributeDescription{ COLOR_CHANNEL, vertexBindingIndex, VK_FORMAT_R32G32B32A32_SFLOAT, 0 }); // color as vec4
            vertexBindingIndex++;
        }
    }
    //PipelineLayout
    vsg::PushConstantRanges pushConstantRanges{
    {VK_SHADER_STAGE_VERTEX_BIT, 0, 128} // projection and modelview matrices
    };
    auto pipelineLayout = vsg::PipelineLayout::create(
        vsg::DescriptorSetLayouts{},
        pushConstantRanges);

    //GraphicsPipelineStates
    vsg::GraphicsPipelineStates pipelineStates{
        vsg::VertexInputState::create(vertexBindingsDescriptions, vertexAttributeDescriptions),
        vsg::InputAssemblyState::create(VK_PRIMITIVE_TOPOLOGY_POINT_LIST),
        vsg::RasterizationState::create(),
        vsg::MultisampleState::create(),
        vsg::ColorBlendState::create(),
        vsg::DepthStencilState::create()
    };

    //GraphicsPipeline
    auto graphicsPipeline = vsg::GraphicsPipeline::create(
        pipelineLayout,
        shaderStages,
        pipelineStates);

    auto bindGraphicsPipeline = vsg::BindGraphicsPipeline::create(graphicsPipeline);
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_pipelineMap[geometryAttributesMask] = bindGraphicsPipeline;
    }
    return bindGraphicsPipeline;
}

vsgPotree::PipelineCache::~PipelineCache()
{
}



