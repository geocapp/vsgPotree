#pragma once

#include <vsg/io/ReaderWriter.h>
#include <iostream>

namespace vsgPotree
{
    class potree : public vsg::Inherit<vsg::ReaderWriter, potree>
    {
    public:
        potree();
        vsg::ref_ptr<vsg::Object> readTileData(const std::vector<uint8_t>&, vsg::ref_ptr<const vsg::Options>) const;

        vsg::ref_ptr<vsg::Object> read(const vsg::Path&, vsg::ref_ptr<const vsg::Options>) const override;
        vsg::ref_ptr<vsg::Object> read(std::istream&, vsg::ref_ptr<const vsg::Options>) const override;
        vsg::ref_ptr<vsg::Object> read(const uint8_t* ptr, size_t size,
                                       vsg::ref_ptr<const vsg::Options> options = {}) const override;

        bool getFeatures(Features& features) const override;
        bool readOptions(vsg::Options& options, vsg::CommandLine& arguments) const override;

    protected:
        virtual ~potree();
        std::set<vsg::Path> _supportedExtensions;
    };
}

EVSG_type_name(vsgPotree::potree);
