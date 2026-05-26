#pragma once

#include <vsg/io/ReaderWriter.h>
#include <iostream>
#include <mutex>
#define PipelineCacheName "PipelineCache"
namespace vsgPotree
{
    enum GeometryAttributes : uint32_t
    {
        VERTEX = (1 << 0),
        NORMAL = (1 << 1),
        NORMAL_OVERALL = (1 << 2),
        TANGENT = (1 << 3),
        TANGENT_OVERALL = (1 << 4),
        COLOR = (1 << 5),
        COLOR_OVERALL = (1 << 6),
        TEXCOORD0 = (1 << 7),
        TEXCOORD1 = (1 << 8),
        TEXCOORD2 = (1 << 9),
        TRANSLATE = (1 << 10),
        TRANSLATE_OVERALL = (1 << 11),
        STANDARD_ATTS = VERTEX | NORMAL | TANGENT | COLOR | TEXCOORD0,
        ALL_ATTS = VERTEX | NORMAL | NORMAL_OVERALL | TANGENT | TANGENT_OVERALL | COLOR | COLOR_OVERALL | TEXCOORD0 | TEXCOORD1 | TEXCOORD2 | TRANSLATE | TRANSLATE_OVERALL
    };
    enum AttributeChannels : uint32_t
    {
        VERTEX_CHANNEL = 0,    // osg 0
        NORMAL_CHANNEL = 1,    // osg 1
        TANGENT_CHANNEL = 2,   //osg 6
        COLOR_CHANNEL = 3,     // osg 2
        TEXCOORD0_CHANNEL = 4, //osg 3
        TEXCOORD1_CHANNEL = 5,
        TEXCOORD2_CHANNEL = 6,
        TRANSLATE_CHANNEL = 7
    };

    class PipelineCache : public vsg::Inherit<vsg::Object, PipelineCache>
    {
    public:
        PipelineCache();
        vsg::ref_ptr<vsg::BindGraphicsPipeline> getOrCreateBindGraphicsPipeline(uint32_t geometryAttributesMask, vsg::ref_ptr<const vsg::Options> options);

    protected:
        virtual ~PipelineCache();

    protected:
        std::map< uint32_t, vsg::ref_ptr<vsg::BindGraphicsPipeline> > m_pipelineMap;
        std::mutex m_mutex;
    };
}

EVSG_type_name(vsgPotree::PipelineCache);
