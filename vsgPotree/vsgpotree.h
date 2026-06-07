#pragma once

#include <vsg/io/ReaderWriter.h>
#include <iostream>

namespace vsgPotree
{
    class Attributes;
    class HNode;
    class PipelineCache;
    class potree : public vsg::Inherit<vsg::ReaderWriter, potree>
    {
    public:
        potree();
        vsg::ref_ptr<vsg::Object> read(const vsg::Path&, vsg::ref_ptr<const vsg::Options>) const override;
        vsg::ref_ptr<vsg::Object> read(std::istream&, vsg::ref_ptr<const vsg::Options>) const override;
        vsg::ref_ptr<vsg::Object> read(const uint8_t* ptr, size_t size,
                                       vsg::ref_ptr<const vsg::Options> options = {}) const override;

        bool getFeatures(Features& features) const override;
        bool readOptions(vsg::Options& options, vsg::CommandLine& arguments) const override;

    protected:
        bool ParseHierarchy(const std::string& hierarchyPath, uint64_t offset, vsg::ref_ptr<HNode>& hNode) const;
        void ParseToHNode(const std::vector<char>& data, vsg::ref_ptr<HNode> hNode) const;
        bool ParseMetadata(const std::string& path, vsg::ref_ptr<Attributes>& attributes) const;
        vsg::ref_ptr<vsg::Node> createTile(vsg::ref_ptr<HNode> hNode, vsg::ref_ptr<Attributes> attributes, const vsg::Path& filePathDir, vsg::ref_ptr<const vsg::Options> options) const;
        vsg::ref_ptr<vsg::Node> createPointCloudNode(const std::vector<char>& data, int pointNum, vsg::ref_ptr<Attributes> attributes, vsg::ref_ptr<vsgPotree::PipelineCache> pipelineCache) const;
        vsg::ref_ptr<vsg::Node> createTileChildren(vsg::ref_ptr<HNode> parentHNode, vsg::ref_ptr<Attributes> attributes, const std::string& filePathDir, vsg::ref_ptr<const vsg::Options> options) const;

    protected:
        virtual ~potree();
        std::set<vsg::Path> _supportedExtensions;
    };
}

EVSG_type_name(vsgPotree::potree);
