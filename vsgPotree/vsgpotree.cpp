#define TINYGLTF_IMPLEMENTATION
//#define TINYGLTF_ENABLE_DRACO
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <map>
#include <vsg/all.h>
#include "vsgpotree.h"
                       vsgPotree::potree::potree() : _supportedExtensions{ ".gltf", ".glb", ".b3dm", ".i3dm" } {}
                       vsgPotree::potree::~potree() {}

vsg::ref_ptr<vsg::Object> vsgPotree::potree::read(const vsg::Path& path, vsg::ref_ptr<const vsg::Options> opt) const
{
    return NULL;
}

vsg::ref_ptr<vsg::Object> vsgPotree::potree::read(std::istream& in, vsg::ref_ptr<const vsg::Options> opt) const
{

    return NULL;
}

vsg::ref_ptr<vsg::Object> vsgPotree::potree::read(const uint8_t* ptr, size_t size,
                                     vsg::ref_ptr<const vsg::Options> opt) const
{
    return NULL;
}

vsg::ref_ptr<vsg::Object> vsgPotree::potree::readTileData(const std::vector<uint8_t>& data,
                                             vsg::ref_ptr<const vsg::Options> opt) const
{
    return NULL;
}

bool vsgPotree::potree::getFeatures(Features& features) const
{
    return true;
}

bool vsgPotree::potree::readOptions(vsg::Options& options, vsg::CommandLine& arguments) const
{
    return false;
}

