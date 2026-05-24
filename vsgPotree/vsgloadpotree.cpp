#include <vsg/all.h>

#include <iostream>
#include <vector>
#include "vsg/core/Array.h"
#include "vsg/core/Inherit.h"

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

////////////////////////////////////////////////////////////////
//
// create point cloud data
//
////////////////////////////////////////////////////////////////

PointCloudContent createPointCloudData()
{
    int pointNum = 100000;
    PointCloudContent content;
    content.vertices = vsg::vec3Array::create(pointNum);
    content.colors = vsg::vec4Array::create(pointNum);

    for (uint32_t i = 0; i < content.count(); i++)
    {
        float x = (float(rand()) / RAND_MAX - 0.5f) * 100.0f;
        float y = (float(rand()) / RAND_MAX - 0.5f) * 100.0f;
        float z = (float(rand()) / RAND_MAX - 0.5f) * 100.0f;

        float r = float(rand()) / RAND_MAX;
        float g = float(rand()) / RAND_MAX;
        float b = float(rand()) / RAND_MAX;
        content.vertices->set(i, vsg::vec3(x, y, z));
        content.colors->set(i, vsg::vec4(r, g, b,1.0));
    }

    return content;
}

vsg::ref_ptr<vsg::Node> createPointCloud()
{
    auto vertexData = createPointCloudData();
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

int main(int argc, char** argv)
{
    // set up defaults and read command line arguments to override them
    vsg::CommandLine arguments(&argc, argv);

    if (argc <= 1)
    {
        std::cout << "Please specify a model to load on command line." << std::endl;
    }

    //vsg::Path filename = arguments[1];

    // set up vsg::Options to pass in filepaths, ReaderWriters and other IO related options to use when reading and writing files.
    auto options = vsg::Options::create();
    options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

    // add OpenSceneGraph's support for reading and writing 3rd party file formats
    options->add(vsg::VSG::create());
    options->add(vsg::spirv::create());

    // load VulkanSceneGraph scene graph
    auto vsg_scene = createPointCloud();

    if (!vsg_scene)
    {
        //std::cout << "vsg::read() could not read : " << filename << std::endl;
        return 1;
    }

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = "VulkanSceneGraph Window";
    windowTraits->width = 800;
    windowTraits->height = 600;

    // create the VulkanSceneGraph viewer and assign window(s) to it
    auto vsg_viewer = vsg::Viewer::create();
    {
        vsg::ref_ptr<vsg::Window> vsg_window(vsg::Window::create(windowTraits));
        if (!vsg_window)
        {
            std::cout << "Could not create window." << std::endl;
            return 1;
        }

        vsg_viewer->addWindow(vsg_window);

        // compute the bounds of the scene graph to help position camera
        vsg::ComputeBounds computeBounds;
        vsg_scene->accept(computeBounds);
        vsg::dvec3 centre = (computeBounds.bounds.min + computeBounds.bounds.max) * 0.5;
        double radius = vsg::length(computeBounds.bounds.max - computeBounds.bounds.min) * 0.6;
        double nearFarRatio = 0.001;
        double farDistance = 100.5 * radius;

        // set up the camera
        auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -1* farDistance, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));
        vsg::ref_ptr<vsg::ProjectionMatrix> perspective;
        if (vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel(vsg_scene->getObject<vsg::EllipsoidModel>("EllipsoidModel")); ellipsoidModel)
        {
            perspective = vsg::EllipsoidPerspective::create(lookAt, ellipsoidModel, 30.0, static_cast<double>(vsg_window->extent2D().width) / static_cast<double>(vsg_window->extent2D().height), nearFarRatio, 0.0);
        }
        else
        {
            perspective = vsg::Perspective::create(30.0, static_cast<double>(vsg_window->extent2D().width) / static_cast<double>(vsg_window->extent2D().height), nearFarRatio * radius, farDistance);
        }
        auto vsg_camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(vsg_window->extent2D()));

        // add close handler to respond to the close window button and pressing escape
        vsg_viewer->addEventHandler(vsg::CloseHandler::create(vsg_viewer));

        vsg_viewer->addEventHandler(vsg::Trackball::create(vsg_camera));

        auto commandGraph = vsg::createCommandGraphForView(vsg_window, vsg_camera, vsg_scene);
        vsg_viewer->assignRecordAndSubmitTaskAndPresentation({ commandGraph });

        vsg_viewer->compile();
    }

    double timeVsg = 0.0;
    int frameNumber = 0;
    // rendering main loop
    auto start1 = std::chrono::high_resolution_clock::now();
    while (true)
    {
        // render VulkanSceneGraph frame   
        {
            if (!vsg_viewer->advanceToNextFrame())
            {
                break;
            }
            vsg_viewer->handleEvents();
            vsg_viewer->update();
            vsg_viewer->recordAndSubmit();
            vsg_viewer->present();
        }
        frameNumber++;
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration1 = end1 - start1;
    timeVsg += duration1.count();
    std::cout << "Vsg Time taken: " << timeVsg << " seconds" << std::endl;
    std::cout << "Vsg Time fps: " << frameNumber / timeVsg << " fps" << std::endl;
    // clean up done automatically thanks to ref_ptr<>
    return 0;
}
