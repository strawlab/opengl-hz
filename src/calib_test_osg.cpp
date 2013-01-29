/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <OpenThreads/ScopedLock>

#include <osg/MatrixTransform>
#include <osg/Projection>
#include <osg/Geometry>
#include <osg/Texture>
#include <osg/TexGen>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/PolygonOffset>
#include <osg/CullSettings>
#include <osg/TextureCubeMap>
#include <osg/TexMat>
#include <osg/MatrixTransform>
#include <osg/Light>
#include <osg/LightSource>
#include <osg/PolygonOffset>
#include <osg/CullFace>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/ArgumentParser>
#include <osg/TextureRectangle>
#include <osg/Texture2D>
#include <osg/Camera>
#include <osg/TexGenNode>
#include <osg/View>
#include <osg/io_utils>

#include <osgGA/TrackballManipulator>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>

#include <osgViewer/ViewerEventHandlers>

#include <stdio.h>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include "util.h"
#include "DisplaySurfaceGeometry.h"
#include "camera_model.h"

osg::Camera* createBG(int width, int height)
{
    // create a camera to set up the projection and model view matrices, and the subgraph to drawn in the HUD
    osg::Camera* camera = new osg::Camera;
    camera->addDescription("background camera");

    // set the projection matrix
    camera->setProjectionMatrix(osg::Matrix::ortho2D(0,width,0,height));

    // set the view matrix
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setViewMatrix(osg::Matrix::identity());

    // only clear the depth buffer
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setClearColor(osg::Vec4(0.0f, 0.3f, 0.0f, 1.0f)); // green

    // draw subgraph after main camera view.
    camera->setRenderOrder(osg::Camera::PRE_RENDER);

    // we don't want the camera to grab event focus from the viewers main camera(s).
    camera->setAllowEventFocus(false);

    return camera;
}

int main(int argc, char**argv) {
    osg::ref_ptr<osg::Group> root = new osg::Group; root->addDescription("root node");

    // set up the texture state.
    std::string filename = "luminance.png";
    osg::Image* image = osgDB::readImageFile(filename);
    if (!image) {
        throw std::ios_base::failure("Could not open image file");
    }
    osg::Camera* bgcam = createBG( image->s(), image->t() );
    root->addChild( bgcam );
    {
        osg::Texture2D* texture = new osg::Texture2D(image);
        osg::Geode* geode = new osg::Geode;
        geode->addDescription("background texture geode");
        {
            osg::Vec3 pos = osg::Vec3(0.0f,0.0f,0.0f);
            osg::Vec3 width(image->s(),0.0f,0.0);
            osg::Vec3 height(0.0,image->t(),0.0);
            osg::Geometry* geometry = osg::createTexturedQuadGeometry(pos,width,height);
            geode->addDrawable(geometry);

            osg::StateSet* stateset = geode->getOrCreateStateSet();
            stateset->setTextureAttributeAndModes(0,texture,osg::StateAttribute::ON);
            stateset->setMode(GL_BLEND,osg::StateAttribute::ON);
            stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        }
        bgcam->addChild(geode);

    }

    DisplaySurfaceGeometry* geometry_parameters = new DisplaySurfaceGeometry( "geom.json" );

    CameraModel* cam1_params = make_real_camera_parameters();

    {
        osg::ref_ptr<osg::Geometry> cyl = geometry_parameters->make_geom();
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(cyl);
        root->addChild(geode);
    }

    osgViewer::Viewer* _viewer = new osgViewer::Viewer;
    _viewer->setSceneData(root.get());

    // construct the viewer.
    _viewer->setUpViewInWindow( 32, 32, image->s(), image->t());
    _viewer->realize();

    float znear=0.1f;
    float zfar=10.0f;
    _viewer->getCamera()->setProjectionMatrix(cam1_params->projection(znear,zfar));
    _viewer->getCamera()->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    osg::Matrixd viewin = cam1_params->view();
    _viewer->getCamera()->setViewMatrix(viewin);
    _viewer->getCamera()->setClearMask( GL_DEPTH_BUFFER_BIT);

    osg::Matrixd proj, view;
    osg::Vec3 dir,m;

    _viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);

    while (!_viewer->done()) {
        _viewer->frame();
    }
    return 0;
}
