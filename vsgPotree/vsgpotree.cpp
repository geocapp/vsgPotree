#include <vsg/all.h>
#include "vsgpotree.h"
#include "Attributes.h"
#include "json.hpp"

namespace vsgPotree
{
    class HNode : public vsg::Inherit<vsg::Object, HNode>
    {
    public:
        enum TYPE
        {
            NORMAL = 0,
            LEAF = 1,
            PROXY = 2,
        };

    public:
        HNode::HNode()
        {
            _name = "";
            _numPoints = 0;
            _childMask = 0;
            _byteOffset = 0;
            _byteSize = 0;
            _type = TYPE::LEAF;
            _children.resize(8);
            for (int i = 0; i < 8; i++)
            {
                _children[i] = NULL;
            }
        }

        std::string   _name = "";
        int      _numPoints = 0;
        uint8_t  _childMask = 0;
        uint64_t _byteOffset = 0;
        uint64_t _byteSize = 0;
        TYPE     _type = TYPE::LEAF;
        std::vector<vsg::ref_ptr<HNode> > _children;
    };
}

vsgPotree::potree::potree() : _supportedExtensions{ ".potree", ".potreechildren"} 
{
}

vsgPotree::potree::~potree()
{
}

void vsgPotree::potree::ParseToHNode(const std::vector<char>& data, vsg::ref_ptr<HNode> hNode) const
{
    hNode->_type = HNode::TYPE(data[0]);
    hNode->_childMask = uint8_t(data[1]);
    hNode->_numPoints = ((uint32_t*)(data.data() + 2))[0];
    hNode->_byteOffset = ((uint64_t*)(data.data() + 6))[0];
    hNode->_byteSize = ((uint64_t*)(data.data() + 14))[0];
}

bool vsgPotree::potree::ParseHierarchy(const std::string& hierarchyPath, int offset, vsg::ref_ptr<HNode>& hNode) const
{
    std::fstream fin(hierarchyPath, std::ios::binary | std::ios::in);
    if (!fin)
    {
        vsg::warn("cannot open file:",hierarchyPath);
        return false;
    }
    fin.seekg(offset, std::ios::beg);
    hNode = new HNode;
    std::vector<char> data(22, 0);
    fin.read(data.data(), 22);
    ParseToHNode(data, hNode);
    std::list<vsg::ref_ptr<HNode> > queues;
    queues.push_back(hNode);
    while (queues.size() > 0)
    {
        vsg::ref_ptr<HNode> hNode = queues.front();
        queues.pop_front();
        if (hNode->_type == HNode::TYPE::PROXY)
        {
            continue;
        }
        for (int i = 0; i < 8; i++)
        {
            if (hNode->_childMask & (1 << i))
            {
                hNode->_children[i] = new HNode;
                std::vector<char> data(22, 0);
                fin.read(data.data(), 22);
                ParseToHNode(data, hNode->_children[i]);
                queues.push_back(hNode->_children[i]);
            }
        }
    }
    fin.close();
    return true;
}

bool vsgPotree::potree::ParseMetadata(const std::string& path, vsg::ref_ptr<Attributes>& attributes) const
{
    attributes = new Attributes;
    std::ifstream file(path);
    if (!file.is_open())
    {
        vsg::warn("canot open file:",path);
        return false;
    }
    nlohmann::json data = nlohmann::json::parse(file);
    file.close();

    //Parse BoundingBox
    auto& bbox = data["boundingBox"];
    std::vector<double> bbox_min = bbox["min"].get<std::vector<double>>();
    std::vector<double> bbox_max = bbox["max"].get<std::vector<double>>();
    attributes->_box = vsg::dbox(vsg::dvec3(bbox_min[0], bbox_min[1], bbox_min[2]), vsg::dvec3(bbox_max[0], bbox_max[1], bbox_max[2]));
    //Parse offset, scale
    std::vector<double> offset = data["offset"].get<std::vector<double>>();
    std::vector<double> scale = data["scale"].get<std::vector<double>>();
    attributes->_posOffset = vsg::dvec3(offset[0], offset[1], offset[2]);
    attributes->_posScale = vsg::dvec3(scale[0], scale[1], scale[2]);
    //Parse attributes array
    std::vector<Attribute> vecAttribute;
    for (auto& attr : data["attributes"])
    {
        Attribute a;
        a.name = attr["name"];
        a.description = attr["description"];
        a.size = attr["size"];
        a.numElements = attr["numElements"];
        a.elementSize = attr["elementSize"];
        a.type = typenameToType(attr["type"]);
        std::vector<double> mindata = attr["min"].get<std::vector<double>>();
        if (mindata.size() == 3)
            a.min = vsg::dvec3(mindata[0], mindata[1], mindata[2]);
        else
            a.min = vsg::dvec3(mindata[0], mindata[0], mindata[0]);
        std::vector<double> maxdata = attr["max"].get<std::vector<double>>();
        if (maxdata.size() == 3)
            a.max = vsg::dvec3(maxdata[0], maxdata[1], maxdata[2]);
        else
            a.max = vsg::dvec3(maxdata[0], maxdata[0], maxdata[0]);
        std::vector<double> scaledata = attr["scale"].get<std::vector<double>>();
        if (scaledata.size() == 3)
            a.scale = vsg::dvec3(scaledata[0], scaledata[1], scaledata[2]);
        else
            a.scale = vsg::dvec3(scaledata[0], scaledata[0], scaledata[0]);
        std::vector<double> offsetdata = attr["offset"].get<std::vector<double>>();
        if (offsetdata.size() == 3)
            a.offset = vsg::dvec3(offsetdata[0], offsetdata[1], offsetdata[2]);
        else
            a.offset = vsg::dvec3(offsetdata[0], offsetdata[0], offsetdata[0]);

        // 特殊处理histogram (只有classification有)
        if (attr.contains("histogram"))
        {
            a.histogram = attr["histogram"].get<std::vector<int64_t>>();
        }
        vecAttribute.push_back(a);
    }
    attributes->setAttribute(vecAttribute);
    return true;
}

vsg::ref_ptr<vsg::Object> vsgPotree::potree::read(const vsg::Path& path, vsg::ref_ptr<const vsg::Options> opt) const
{
    if (!vsg::compatibleExtension(path, opt, _supportedExtensions))
    {
        return NULL;
    }
    vsg::Path ext = vsg::fileExtension(path);
    vsg::Path filePathDir = vsg::removeExtension(path);
    if (ext == ".potreechildren")//childNode
    {

    }
    if (ext == ".potree")//rootNode
    {
        vsg::Path hierarchyPath = filePathDir/"hierarchy.bin";
        vsg::ref_ptr<HNode> hNode;
        if (ParseHierarchy(hierarchyPath.string(), 0, hNode))
        {
            vsg::ref_ptr<Attributes> attributes = NULL;
            vsg::Path metadataPath = filePathDir / "metadata.json";
            ParseMetadata(metadataPath.string(), attributes);
            return createTile(hNode, attributes, filePathDir, opt);
        }
    }

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

bool vsgPotree::potree::getFeatures(Features& features) const
{
    return true;
}

bool vsgPotree::potree::readOptions(vsg::Options& options, vsg::CommandLine& arguments) const
{
    return false;
}


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

struct PointCloudContent
{
    vsg::ref_ptr<vsg::vec3Array> vertices;
    vsg::ref_ptr<vsg::vec4Array> colors;

    uint32_t count() const { return vertices ? static_cast<uint32_t>(vertices->size()) : 0; }
};

////////////////////////////////////////////////////////////////
//
// shaders
//
////////////////////////////////////////////////////////////////

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

vsg::ref_ptr<vsg::Node> vsgPotree::potree::createPointCloudNode(const std::vector<char>& data, int pointNum, vsg::ref_ptr<Attributes> attributes) const
{
    PointCloudContent vertexData;
    vertexData.vertices = vsg::vec3Array::create(pointNum);
    vertexData.colors = vsg::vec4Array::create(pointNum);
    vsg::dvec3 center = (attributes->_box.min + attributes->_box.max) * 0.5;
    for (int i = 0; i < pointNum; i++)
    {
        const char* pData = data.data() + (attributes->_bytes * i);
        for (int j = 0; j < attributes->_list.size(); j++)
        {
            if ("position" == attributes->_list[j].name)
            {
                if (INT32 == attributes->_list[j].type || UINT32 == attributes->_list[j].type)
                {
                    int* pDataInt = (int*)pData;
                    vsg::vec3 position = vsg::vec3(attributes->_posOffset.x + attributes->_posScale.x * pDataInt[0] - center.x,
                        attributes->_posOffset.y + attributes->_posScale.y * pDataInt[1] - center.y,
                        attributes->_posOffset.z + attributes->_posScale.z * pDataInt[2] - center.z);
                    vertexData.vertices->set(i, position);
                }
                else
                {
                    vsg::fatal("position type is:",attributes->_list[j].type,", please improve");
                }
            }
            else if ("rgb" == attributes->_list[j].name)
            {
                if (INT16 == attributes->_list[j].type || UINT16 == attributes->_list[j].type)
                {
                    uint16_t* pDataUInt16 = (uint16_t*)pData;
                    vsg::vec4 color = vsg::vec4((pDataUInt16[0] >> 8)/256,
                        (pDataUInt16[1] >> 8) / 256,
                           (pDataUInt16[2] >> 8) / 256,
                        1.0);
                    vertexData.colors->set(i,color);
                }
                else
                {
                    vsg::fatal("color type is:" ,attributes->_list[j].type,", please improve");
                }
            }
            pData += attributes->_list[j].size;
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
            vertexAttributeDescriptions.push_back(VkVertexInputAttributeDescription{ VERTEX_CHANNEL, vertexBindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0 });
            vertexBindingIndex++;
        }

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

    auto stateGroup = vsg::StateGroup::create();

    stateGroup->add(vsg::BindGraphicsPipeline::create(graphicsPipeline));

    auto commands = vsg::Commands::create();

    commands->addChild(
        vsg::BindVertexBuffers::create(
            0,
            vsg::DataList{ vertexData.vertices,vertexData.colors }));

    commands->addChild(
        vsg::Draw::create(
            static_cast<uint32_t>(vertexData.count()),
            1,
            0,
            0));

    stateGroup->addChild(commands);
    return stateGroup;
}

vsg::ref_ptr<vsg::Node> vsgPotree::potree::createTile(vsg::ref_ptr<HNode> hNode, vsg::ref_ptr<Attributes> attributes, const vsg::Path& filePathDir, vsg::ref_ptr<const vsg::Options> options) const
{
    if (!hNode)
    {
        return NULL;
    }
    vsg::Path binPath = filePathDir / "octree.bin";
    if (HNode::TYPE::PROXY == hNode->_type)
    {
        vsg::Path hierarchyPath =  filePathDir / "hierarchy.bin";
        vsg::ref_ptr<HNode> hChunkNode = NULL;
        if (ParseHierarchy(hierarchyPath.string(), hNode->_byteOffset, hChunkNode))
        {
            return createTile(hChunkNode, attributes, filePathDir, options);
        }
    }
    else
    {
        std::fstream fin(binPath, std::ios::binary | std::ios::in);
        if (!fin)
        {
            vsg::warn("cannot open file:",binPath);
            return NULL;
        }
        fin.seekg(hNode->_byteOffset, std::ios::beg);
        std::vector<char> data(hNode->_byteSize, 0);
        fin.read(data.data(), hNode->_byteSize);
        fin.close();
        vsg::ref_ptr<vsg::Node> pointcloudNode = createPointCloudNode(data, hNode->_numPoints, attributes);
        //if (0 != hNode->_childMask)
        //{
        //    vsg::ref_ptr<vsg::PagedLOD> plod = new vsg::PagedLOD;
        //    plod->addChild(pointcloudNode);
        //    osgDB::Options* childOpt = new osgDB::Options;
        //    osg::ref_ptr<PotreeContainer> parentContainer = new PotreeContainer(hNode, attributes);
        //    parentContainer->setName("ParentTile");
        //    childOpt->getOrCreateUserDataContainer()->addUserObject(parentContainer);
        //    plod->setDatabaseOptions(childOpt);
        //    plod->setFileName(1, filePathDir + "." + std::to_string(hNode->_byteOffset) + ".pchildren");

        //    if (pointcloudNode.valid())
        //    {
        //        osg::BoundingSphered bound2;
        //        osg::BoundingSphere bound0 = pointcloudNode->getBound();
        //        bound2.expandBy(osg::BoundingSphered(bound0.center(), bound0.radius()));
        //        plod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
        //        plod->setCenter(bound2.center());
        //        plod->setRadius(bound2.radius());
        //    }
        //    else
        //    {
        //        OSG_WARN << "[ReaderWriterPotree] Missing <boundingVolume>?" << std::endl;
        //    }
        //    plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
        //    plod->setRange(0, 0.0f, FLT_MAX);
        //    double radius = plod->getRadius();
        //    //Assuming the screen height in pixels is 1080.
        //    //The maximum cross-sectional area of the data on the screen is 4 * radius * radius
        //    //The field of view is 60 degrees, with tan(30) = 0.5629
        //    //assuing every points is rendered as a 1 pixel square
        //    double range = 1080 * std::sqrt(4 * radius * radius / hNode->_numPoints) / 2 / 0.5629;
        //    plod->setRange(1, 0, range);
        //    return plod.release();
        //}
        //else
        {
            return pointcloudNode;
        }
    }

    return NULL;
}



