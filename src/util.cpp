/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "util.h"

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>

#include <sstream>

std::string join_path(std::string a,std::string b) {
    // roughly inspired by Python's os.path.join
    char pathsep = '/'; // TODO: FIXME: not OK on Windows.
    if (a.at(a.size()-1)==pathsep) {
        return a+b;
    } else {
        return a+std::string("/")+b;
    }
}

// load source from a file.
void LoadShaderSource( osg::Shader* shader, const std::string& fileName )
{
    std::string fqFileName = osgDB::findDataFile(fileName);
    if( fqFileName.length() != 0 )
    {
        shader->loadShaderSourceFromFile( fqFileName.c_str() );
    }
    else
    {
        std::stringstream ss;
        ss << "File \"" << fileName << "\" not found.";
        throw std::ios_base::failure(ss.str());
    }
}

osg::Camera* createHUD()
{
    // create a camera to set up the projection and model view matrices, and the subgraph to drawn in the HUD
    osg::Camera* camera = new osg::Camera;

    // set the projection matrix
    camera->setProjectionMatrix(osg::Matrix::ortho2D(0,1.0,0,1.0));

    // set the view matrix
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setViewMatrix(osg::Matrix::identity());

    // only clear the depth buffer
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);

    // draw subgraph after main camera view.
    camera->setRenderOrder(osg::Camera::POST_RENDER);

    // we don't want the camera to grab event focus from the viewers main camera(s).
    camera->setAllowEventFocus(false);

    return camera;
}

osg::Group* make_textured_quad(osg::Texture* texture,
                               float zpos,
                               float max_tc_width, float max_tc_height,
                               float left, float bottom,
                               float width, float height) {
    osg::Group* group = new osg::Group;
    group->addDescription("textured quad group");

    osg::Geode* geode = new osg::Geode();
    {
        // make quad
        osg::Vec3Array* vertices = new osg::Vec3Array;
        osg::Vec2Array* tcs = new osg::Vec2Array;
        vertices->push_back(  osg::Vec3(left, bottom, zpos) ); tcs->push_back(osg::Vec2(0.0,0.0));
        vertices->push_back(  osg::Vec3(left+width, bottom, zpos) ); tcs->push_back(osg::Vec2(max_tc_width,0.0));
        vertices->push_back(  osg::Vec3(left+width, bottom+height, zpos) ); tcs->push_back(osg::Vec2(max_tc_width,max_tc_height));
        vertices->push_back(  osg::Vec3(left, bottom+height, zpos) ); tcs->push_back(osg::Vec2(0.0,max_tc_height));

        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));

        osg::ref_ptr<osg::Geometry> this_geom = new osg::Geometry();
        this_geom->setVertexArray(vertices);
        this_geom->setTexCoordArray(0,tcs);
        this_geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));
        this_geom->setColorArray(colors.get());
        this_geom->setColorBinding(osg::Geometry::BIND_OVERALL);
        geode->addDrawable(this_geom);
    }
    group->addChild(geode);

    osg::StateSet* ss = group->getOrCreateStateSet();
    ss->setTextureAttributeAndModes(0,texture,osg::StateAttribute::ON);
    ss->setMode(GL_LIGHTING,osg::StateAttribute::OFF);

    return group;
}
