/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "DisplaySurfaceGeometry.h"

#include <iostream>
#include <fstream>

#include <osg/Geometry>

#include <stdio.h>
#include <jansson.h>

#include <stdexcept>
#include <sstream>

class CylinderModel : public GeomModel {
public:
    CylinderModel(float radius, osg::Vec3 base, osg::Vec3 axis) :
        _radius(radius), _base(base), _axis(axis), _n_segments(256) {

        osg::Vec3 unit_z = osg::Vec3(0.0, 0.0, 1.0);

        // find vector from bottom to top and then normalize it
        osg::Vec3 normax = _axis;
        normax.normalize();

        _matrix = osg::Matrix::rotate( unit_z, normax ); // from unit_z to normax
        _height = _axis.length();
    }

    osg::Vec3 texcoord2worldcoord( osg::Vec2 tc ) {
        // keep in sync with simple_geom.py
        double frac_theta = tc[0];
        double frac_height = tc[1];

        double angle = frac_theta * 2.0*osg::PI + osg::PI;
        float c = cosf(angle);
        float s = sinf(angle);
        double r = _radius;

        return (osg::Vec3(c*r,s*r,frac_height*_height))*_matrix + _base;
    }

    osg::Vec3 texcoord2normal( osg::Vec2 tc ) {
        double frac_theta = tc[0];

        double angle = frac_theta * 2.0*osg::PI;
        float c = cosf(angle);
        float s = sinf(angle);
        return osg::Vec3(c,s,0)*_matrix;
    }

    osg::ref_ptr<osg::Geometry> make_geom(bool texcoord_colors) {
        osg::ref_ptr<osg::Geometry> this_geom = new osg::Geometry;
        {
            double fracDelta = 1.0/(double)_n_segments;
            double frac=0.0;

            osg::Vec3Array* vertices = new osg::Vec3Array;
            osg::Vec3Array* normals = new osg::Vec3Array;
            osg::Vec2Array* tc = new osg::Vec2Array; // cylindrical coordinates
            osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;

            for(unsigned int bodyi=0;
                bodyi<=_n_segments;
                ++bodyi,frac+=fracDelta)
                {
                    {
                        osg::Vec2 tci = osg::Vec2( frac, 1.0);
                        vertices->push_back( texcoord2worldcoord(tci) );
                        normals->push_back( texcoord2normal(tci) );
                        tc->push_back( tci );
                        if (texcoord_colors) {
                            colors->push_back( osg::Vec4( tci[0], tci[1], 0.0, 1.0 ) );
                        }
                    }
                    {
                        osg::Vec2 tci = osg::Vec2( frac, 0.0);
                        vertices->push_back( texcoord2worldcoord(tci) );
                        normals->push_back( texcoord2normal(tci) );
                        tc->push_back( tci );
                        if (texcoord_colors) {
                            colors->push_back( osg::Vec4( tci[0], tci[1], 0.0, 1.0 ) );
                        }
                    }
                }

            if (!texcoord_colors) {
                colors->push_back(osg::Vec4(0.0f,1.0f,0.0f,1.0f));
            }
            this_geom->setVertexArray(vertices);
            this_geom->setNormalArray(normals);
            this_geom->setTexCoordArray(0,tc);
            this_geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS,0,2*_n_segments+2));
            this_geom->setColorArray(colors.get());
            if (texcoord_colors) {
                this_geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            } else {
                this_geom->setColorBinding(osg::Geometry::BIND_OVERALL);
            }
        }
        return this_geom;
    }

    KeyPointMap get_key_points() {
        std::map<std::string, osg::Vec3> result;

        result["base"] = _base;
        result["top"] = _base+_axis;
        result["(0,0)"] = texcoord2worldcoord( osg::Vec2(0,0));
        result["(0,1)"] = texcoord2worldcoord( osg::Vec2(0,1));
        return result;
    }

private:
    double _radius;
    osg::Vec3 _base;
    osg::Vec3 _axis; // includes height

    unsigned int _n_segments;

    // derived from above:
    osg::Matrix _matrix;
    double _height;
};

class SphereModel : public GeomModel {
public:
    SphereModel(float radius, osg::Vec3 center) :
        _radius(radius), _center(center), _n_az(20), _n_el(12) {}

    osg::Vec3 texcoord2worldcoord( osg::Vec2 tc ) {
        // keep in sync with simple_geom.py
        double frac_az = tc[0];
        double frac_el = tc[1];

        double az = frac_az * 2.0*osg::PI;
        double el = frac_el*osg::PI - osg::PI/2.0;

        double ca = cos(az);
        double sa = sin(az);

        double ce = cos(el);
        double se = sin(el);

        double r = _radius;

        return osg::Vec3(r*ca*ce, r*sa*ce, r*se);
    }

    osg::Vec3 texcoord2normal( osg::Vec2 tc ) {
        double frac_az = tc[0];
        double frac_el = tc[1];

        double az = frac_az * 2.0*osg::PI;
        double el = frac_el*osg::PI - osg::PI/2.0;

        double ca = cos(az);
        double sa = sin(az);

        double ce = cos(el);
        double se = sin(el);

        return osg::Vec3(ca*ce, sa*ce, se);
    }

    osg::ref_ptr<osg::Geometry> make_geom(bool texcoord_colors) {
        osg::ref_ptr<osg::Geometry> this_geom = new osg::Geometry;
        {
            double frac_az_Delta = 1.0/(double)_n_az;
            double frac_el_Delta = 1.0/(double)_n_el;

            double frac_el=0.0;

            osg::Vec3Array* vertices = new osg::Vec3Array;
            osg::Vec3Array* normals = new osg::Vec3Array;
            osg::Vec2Array* tc = new osg::Vec2Array; // cylindrical coordinates
            osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
            int idx=0;

            // This is quick and dirty, not elegant. Many vertices
            // drawn twice or more times, no triangle strips, etc. It
            // should be improved.

            for(unsigned int bodyi=0;
                bodyi<_n_el;
                ++bodyi,frac_el+=frac_el_Delta)
            {
                double frac_az=0.0;
                double frac_el2=frac_el+frac_el_Delta;

                osg::DrawElementsUInt* sphere_strip =
                    new osg::DrawElementsUInt(osg::PrimitiveSet::QUAD_STRIP, 0);

                for(unsigned int bodyj=0;
                    bodyj<=_n_az;
                    ++bodyj,frac_az+=frac_az_Delta)
                {

                    {
                        osg::Vec2 tci = osg::Vec2( frac_az, frac_el2);
                        vertices->push_back( texcoord2worldcoord(tci) );
                        normals->push_back( texcoord2normal(tci) );
                        tc->push_back( tci );
                        if (texcoord_colors) {
                            colors->push_back( osg::Vec4( tci[0], tci[1], 0.0, 1.0 ) );
                        }
                        sphere_strip->push_back(idx);
                        idx++;
                    }
                    {
                        osg::Vec2 tci = osg::Vec2( frac_az, frac_el);
                        vertices->push_back( texcoord2worldcoord(tci) );
                        normals->push_back( texcoord2normal(tci) );
                        tc->push_back( tci );
                        if (texcoord_colors) {
                            colors->push_back( osg::Vec4( tci[0], tci[1], 0.0, 1.0 ) );
                        }
                        sphere_strip->push_back(idx);
                        idx++;
                    }
                    this_geom->addPrimitiveSet(sphere_strip);
                }
            }
            if (!texcoord_colors) {
                colors->push_back(osg::Vec4(1.0f,1.0f,1.0f,1.0f));
            }
            this_geom->setVertexArray(vertices);
            this_geom->setNormalArray(normals);
            this_geom->setTexCoordArray(0,tc);
            this_geom->setColorArray(colors.get());
            if (texcoord_colors) {
                this_geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            } else {
                this_geom->setColorBinding(osg::Geometry::BIND_OVERALL);
            }
        }
        return this_geom;
    }

    KeyPointMap get_key_points() {
        std::map<std::string, osg::Vec3> result;

        result["center"] = _center;
        result["(0,0)"] = texcoord2worldcoord( osg::Vec2(0,0));
        result["(0,0.5)"] = texcoord2worldcoord( osg::Vec2(0,0.5));
        result["(0,1)"] = texcoord2worldcoord( osg::Vec2(0,1));
        return result;
    }

private:
    double _radius;
    osg::Vec3 _center;

    unsigned int _n_az;
    unsigned int _n_el;
};

osg::Vec3 parse_vec3( json_t *root) {
    float x,y,z;

    json_t *x_json = json_object_get(root, "x");
    if(!json_is_number(x_json)){
        throw std::runtime_error("Error parsing vec3: x is not a number");
    }
    x = json_number_value( x_json );

    json_t *y_json = json_object_get(root, "y");
    if(!json_is_number(y_json)){
        throw std::runtime_error("Error parsing vec3: y is not a number");
    }
    y = json_number_value( y_json );

    json_t *z_json = json_object_get(root, "z");
    if(!json_is_number(z_json)){
        throw std::runtime_error("Error parsing vec3: z is not a number");
    }
    z = json_number_value( z_json );

    return osg::Vec3(x,y,z);
}

DisplaySurfaceGeometry::DisplaySurfaceGeometry(const char *fname) {
    json_t *root;
    json_error_t error;

    root = json_load_file(fname, 0, &error);
    if(!root) {
        std::ostringstream os;
        os << "Could not load json line " << error.line << ": " << error.text;
        throw std::runtime_error(os.str());
    }

    parse_json(root);
}

void DisplaySurfaceGeometry::parse_json(json_t *root) {
    json_t *model_json = json_object_get(root, "model");
    if(!json_is_string(model_json)){
        throw std::runtime_error("parsing model: expected string");
    }
    std::string model( json_string_value( model_json ) );
    if (model==std::string("cylinder")) {

        json_t *radius_json = json_object_get(root, "radius");
        if(!json_is_number(radius_json)){
            throw std::runtime_error("cylinder parsing radius: expected number");
        }
        double radius = json_number_value( radius_json );

        json_t *base_json = json_object_get(root, "base");
        if(!json_is_object(base_json)){
            throw std::runtime_error("cylinder parsing base: expected object");
        }
        osg::Vec3 base = parse_vec3( base_json );

        json_t *axis_json = json_object_get(root, "axis");
        if(!json_is_object(axis_json)){
            throw std::runtime_error("cylinder parsing axis: expected object");
        }
        osg::Vec3 axis = parse_vec3( axis_json );

        _geom = new CylinderModel(radius,base,axis);
    } else if (model==std::string("sphere")) {

        json_t *radius_json = json_object_get(root, "radius");
        if(!json_is_number(radius_json)){
            throw std::runtime_error("sphere parsing radius: expected number");
        }
        double radius = json_number_value( radius_json );

        json_t *center_json = json_object_get(root, "center");
        if(!json_is_object(center_json)){
            throw std::runtime_error("sphere parsing center: expected object");
        }
        osg::Vec3 center = parse_vec3( center_json );

        _geom = new SphereModel(radius,center);
    } else {
        std::ostringstream os;
        os << "unknown model " << model;
        throw std::runtime_error(os.str());
    }

    json_decref(root);
}

osg::ref_ptr<osg::Geometry> DisplaySurfaceGeometry::make_geom(bool texcoord_colors) {
    return _geom->make_geom(texcoord_colors);
};

KeyPointMap DisplaySurfaceGeometry::get_key_points() {
    return _geom->get_key_points();
}
