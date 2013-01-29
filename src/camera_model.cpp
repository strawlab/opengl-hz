/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "camera_model.h"
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/MatrixTransform>

#include <stdexcept>
#include <assert.h>

CameraModel::CameraModel(unsigned int width, unsigned int height, bool y_up)  :
	_width(width), _height(height), _y_up(y_up), intrinsic_valid(false), extrinsic_valid(false)
{
}

// get extrinsic parameter information
osg::Vec3 CameraModel::eye() const { if (!extrinsic_valid) {throw "invalid extrinsic";} return _eye;}
osg::Vec3 CameraModel::center() const {if (!extrinsic_valid) {throw "invalid extrinsic";} return _center;}
osg::Vec3 CameraModel::up() const {if (!extrinsic_valid) {throw "invalid extrinsic";} return _up;}

osg::Matrixd CameraModel::view() const {
    if (!extrinsic_valid) {throw "invalid extrinsic";}
	osg::Matrixd result = osg::Matrixd::lookAt(eye(),
                                               center(),
                                               up());
	return result;
}

osg::Matrixd CameraModel::projection(float znear, float zfar) const {
	// See http://strawlab.org/2011/11/05/augmented-reality-with-OpenGL/
    if (!intrinsic_valid) {throw "invalid intrinsic";}

	float depth = zfar - znear;
	float q = -(zfar + znear) / depth;
	float qn = -2 * (zfar * znear) / depth;

	float x0=0;
	float y0=0;

	osg::Matrixd p;
	float width=_width;
	float height=_height;

	if (_y_up) {
		p=osg::Matrixf( 2*_K00/ width, -2*_K01/ width, (-2*_K02+ width+2*x0)/ width, 0,
						0,             -2*_K11/height, (-2*_K12+height+2*y0)/height, 0,
						0,              0,             q,                            qn,
						0,              0,             -1,                           0);
	}
	else {
		p=osg::Matrixf( 2*_K00/ width, -2*_K01/ width, (-2*_K02+ width+2*x0)/ width, 0,
						0,              2*_K11/height, ( 2*_K12-height+2*y0)/height, 0,
						0,              0,             q,                            qn,
						0,              0,             -1,                           0);
 	}
	osg::Matrixd pT;
	for (int i=0; i<4; i++) {
		for (int j=0; j<4; j++) {
			pT(i,j) = p(j,i);
		}
	}
	return pT;
}

osg::ref_ptr<osg::Group> CameraModel::make_rendering(float size) const {
    if (!intrinsic_valid) {
        throw std::runtime_error("need valid intrinsics to make rendering");
    }
    if (!extrinsic_valid) {
        throw std::runtime_error("need valid extrinsics to make rendering");
    }

    // Projection and ModelView matrices
    osg::Matrixd proj;
    osg::Matrixd mv;

	proj = projection(size*0.1,size);
	mv.makeLookAt( _eye, _center, _up );

    // Get near and far from the Projection matrix.
    const double near = proj(3,2) / (proj(2,2)-1.0);
    const double far = proj(3,2) / (1.0+proj(2,2));

    // Get the sides of the near plane.
    const double nLeft = near * (proj(2,0)-1.0) / proj(0,0);
    const double nRight = near * (1.0+proj(2,0)) / proj(0,0);
    const double nTop = near * (1.0+proj(2,1)) / proj(1,1);
    const double nBottom = near * (proj(2,1)-1.0) / proj(1,1);

    // Get the sides of the far plane.
    const double fLeft = far * (proj(2,0)-1.0) / proj(0,0);
    const double fRight = far * (1.0+proj(2,0)) / proj(0,0);
    const double fTop = far * (1.0+proj(2,1)) / proj(1,1);
    const double fBottom = far * (proj(2,1)-1.0) / proj(1,1);

    // Our vertex array needs only 9 vertices: The origin, and the
    // eight corners of the near and far planes.
    osg::Vec3Array* v = new osg::Vec3Array;
    v->resize( 9 );
    (*v)[0].set( 0., 0., 0. );
    (*v)[1].set( nLeft, nBottom, -near );
    (*v)[2].set( nRight, nBottom, -near );
    (*v)[3].set( nRight, nTop, -near );
    (*v)[4].set( nLeft, nTop, -near );
    (*v)[5].set( fLeft, fBottom, -far );
    (*v)[6].set( fRight, fBottom, -far );
    (*v)[7].set( fRight, fTop, -far );
    (*v)[8].set( fLeft, fTop, -far );

    osg::Geometry* geom = new osg::Geometry;
    geom->setUseDisplayList( false );
    geom->setVertexArray( v );

    osg::Vec4Array* c = new osg::Vec4Array;
    c->push_back( osg::Vec4( 1., 1., 1., 1. ) );
    geom->setColorArray( c );
    geom->setColorBinding( osg::Geometry::BIND_OVERALL );

    GLushort idxLines[8] = {
        0, 5, 0, 6, 0, 7, 0, 8 };
    GLushort idxLoops0[4] = {
        1, 2, 3, 4 };
    GLushort idxLoops1[4] = {
        5, 6, 7, 8 };
    geom->addPrimitiveSet( new osg::DrawElementsUShort( osg::PrimitiveSet::LINES, 8, idxLines ) );
    geom->addPrimitiveSet( new osg::DrawElementsUShort( osg::PrimitiveSet::LINE_LOOP, 4, idxLoops0 ) );
    geom->addPrimitiveSet( new osg::DrawElementsUShort( osg::PrimitiveSet::LINE_LOOP, 4, idxLoops1 ) );

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable( geom );

    geode->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED );


    // Create parent MatrixTransform to transform the view volume by
    // the inverse ModelView matrix.
    osg::MatrixTransform* mt = new osg::MatrixTransform;
    mt->setMatrix( osg::Matrixd::inverse( mv ) );
    mt->addChild( geode );

	osg::Group* group = new osg::Group;
	group->addDescription("camera viewer");
	group->addChild(mt);
	return group;
}

osg::Vec3 CameraModel::project_pixel_to_camera_frame(osg::Vec2 uv, bool distorted, double distance ) {
    if (!intrinsic_valid) {
        throw std::runtime_error("need valid intrinsics to project pixel to camera frame");
    }

    if (distorted) {
        throw std::runtime_error("distortion not yet implemented");
    }
    double fx, fy, cx, cy, Tx, Ty, x,y,z;

    fx = _K00;            cx = _K02;  Tx = 0.0;
                 fy=_K11; cy = _K12;  Ty = 0.0;

    x = (uv[0] - cx - Tx) / fx;
    y = (uv[1] - cy - Ty) / fy;
    z = -1.0f; // -1 because osg and ROS/OpenCV coords are flipped

    osg::Vec3 result(x,y,z);
    result.normalize();
    result *= distance;
    return result;
}

osg::Matrix CameraModel::get_rot() const {
    osg::Vec3d lv( _center - _eye );

    osg::Vec3d f( lv );
    f.normalize();
    osg::Vec3d s( f^_up );
    s.normalize();
    osg::Vec3d u( s^f );
    u.normalize();

    osg::Matrixd rotation_matrix( s[0], u[0], -f[0], 0.0f,
                            s[1], u[1], -f[1], 0.0f,
                            s[2], u[2], -f[2], 0.0f,
                            0.0f, 0.0f,  0.0f, 1.0f );
    return rotation_matrix;
}

osg::Matrix CameraModel::get_rot_inv() const {
    osg::Matrix result;
    // OSG doesn't have a transpose matrix function?
    result.invert( get_rot() );
    return result;
}

osg::Vec3 CameraModel::get_translation() const {
    osg::Vec4 eye( _eye[0], _eye[1], _eye[2], 1.0);
    osg::Vec4 t = -(eye*get_rot());
    assert(t[3]==1.0);
    osg::Vec3 result(t[0], t[1], t[2]);
    return result;
}

osg::Vec3 CameraModel::project_camera_frame_to_3d(osg::Vec3 xyz_c ) {
    osg::Vec3 diff = xyz_c-get_translation();
    osg::Vec3 world_coords = diff*get_rot_inv();
    return world_coords;
}

void CameraModel::set_intrinsic( double K00, double K01, double K02,
                                 double K11, double K12 ) {
    _K00 = K00;
    _K01 = K01;
    _K02 = K02;
    _K11 = K11;
    _K12 = K12;
    intrinsic_valid = true;
}

void CameraModel::set_extrinsic( osg::Vec3 eye, osg::Vec3 center, osg::Vec3 up ) {
    _eye = eye;
    _center = center;
    _up = up;
    extrinsic_valid = true;
}

CameraModel* make_real_camera_parameters() {
	// this is just a stub until we get real parameter loading code in here.
	osg::Vec3 eye = osg::Vec3(-0.708471152493,-1.4184181224,1.30394218099);
	osg::Vec3 center = osg::Vec3(-0.280027771115,-0.647764804425,0.832211609118);
	osg::Vec3 up = osg::Vec3(-0.197303085284,-0.429683565144,-0.881160329556);
	float K00 = 604.39963621;
	float K01 = -7.33740535;
	float K02 = 356.25995387;
	float K11 = 578.11306274;
	float K12 = 257.36283644;
	bool y_up=false;

    CameraModel* result = new CameraModel(752,480,y_up);
    result->set_intrinsic(K00,K01,K02,K11,K12);
    result->set_extrinsic(eye,center,up);

	return result;
}
