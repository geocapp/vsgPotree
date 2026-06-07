#include <vsg/all.h>

#include <iostream>
#include <vector>
#include "vsg/core/Array.h"
#include "vsg/core/Inherit.h"
#include "vsg/io/read.h"
#include "vsgPipelineCache.h"
#include "vsgpotree.h"

int main(int argc, char** argv)
{
    // set up defaults and read command line arguments to override them
    vsg::CommandLine arguments(&argc, argv);

    if (argc <= 1)
    {
        std::cout << "Please specify a model to load on command line." << std::endl;
    }

    vsg::Path filename = arguments[1];
    bool reportAverageFrameRate = arguments.read("--fps");
    // set up vsg::Options to pass in filepaths, ReaderWriters and other IO related options to use when reading and writing files.
    vsg::ref_ptr<vsg::Options> options = vsg::Options::create();
    options->fileCache = vsg::getEnv("VSG_FILE_CACHE");
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

    // add OpenSceneGraph's support for reading and writing 3rd party file formats
    options->add(vsg::VSG::create());
    options->add(vsg::spirv::create());
    options->add(vsgPotree::potree::create());
    vsg::ref_ptr<vsgPotree::PipelineCache> pc = vsgPotree::PipelineCache::create();
    options->setObject(PipelineCacheName, pc);
    // load VulkanSceneGraph scene graph
    //auto vsg_scene = createPointCloudNode();
    vsg::ref_ptr<vsg::Object> vsg_node = vsg::read(filename, options);
    vsg::ref_ptr<vsg::Node> vsg_scene;
    if (vsg_node)
    {
        vsg_scene = vsg_node.cast<vsg::Node>();
    }
    if (!vsg_scene)
    {
        //std::cout << "vsg::read() could not read : " << filename << std::endl;
        return 1;
    }

    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = "vsgPotree Window";
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
        double farDistance = 3.5 * radius;

        // set up the camera
        auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -1* farDistance, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));
        vsg::ref_ptr<vsg::ProjectionMatrix> perspective;
        if (vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel(vsg_scene->getObject<vsg::EllipsoidModel>("EllipsoidModel")); ellipsoidModel)
        {
            perspective = vsg::EllipsoidPerspective::create(lookAt, ellipsoidModel, 30.0, static_cast<double>(vsg_window->extent2D().width) / static_cast<double>(vsg_window->extent2D().height), nearFarRatio, 0.0);
        }
        else
        {
            perspective = vsg::Perspective::create(30.0, static_cast<double>(vsg_window->extent2D().width) / static_cast<double>(vsg_window->extent2D().height), nearFarRatio * radius, 1.5*farDistance);
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
