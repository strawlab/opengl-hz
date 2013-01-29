#!/usr/bin/env python

# stdlib imports
import os

#other imports
import numpy as np
import scipy.misc

import matplotlib.pyplot as plt
import matplotlib.cm

from calib_test_utils import generate_cyl_points, decompose, \
     convert_hz_intrinsic_to_opengl_projection

R2D = 180.0/np.pi

def debug(val):
    if 0:
        print repr(val)

def main():
    np.set_printoptions(precision=5, linewidth=200)

    window_coords = 'y down'

    src_dir = os.path.split(os.path.abspath(__file__))[0]
    data_dir = os.path.join(src_dir,'..','data')
    pmat = np.loadtxt( os.path.join(data_dir, 'cameramatrix.txt') )
    luminance = scipy.misc.imread( os.path.join(data_dir, 'luminance.png' ) )
    img_height, img_width = luminance.shape

    cyl_points_3d_h = generate_cyl_points(homog=True,n_segs=50)
    debug('cyl_points_3d_h')
    debug(cyl_points_3d_h)

    if 1:
        calib = decompose(pmat)
        cyl_points_3d_eye = np.dot( calib['extrinsic'], cyl_points_3d_h )
        debug('cyl_points_3d_eye')
        debug(cyl_points_3d_eye)

        if 1:
            e = np.vstack((calib['extrinsic'],[[0,0,0,1]])) # These HZ eye coords have +Z in front of camera.
            coord_xform = np.eye(4)
            coord_xform[1,1]=-1 # flip Y coordinate (HZ camera has +y going down, GL has +y going up)
            coord_xform[2,2]=-1 # flip Z coordinate (HZ camera looks at +z, GL looks at -z)
            #debug('e')
            #debug(e)
            e2 = np.dot( coord_xform, e)
            #debug('e2')
            #debug(e2)

            cyl_points_3d_eye_hz_h = np.dot( e, cyl_points_3d_h )
            cyl_points_3d_eye_opengl_h = np.dot( e2, cyl_points_3d_h )

            debug('cyl_points_3d_eye_hz_h')
            debug(cyl_points_3d_eye_hz_h)

            debug('cyl_points_3d_eye_opengl_h')
            debug(cyl_points_3d_eye_opengl_h)

    if 1:
        cyl_points_2d_h = np.dot(calib['intrinsic'], cyl_points_3d_eye)
        cyl_points_2d = cyl_points_2d_h[:2]/cyl_points_2d_h[2]

    else:
        cyl_points_2d_h = np.dot(pmat, cyl_points_3d_h)
        cyl_points_2d = cyl_points_2d_h[:2]/cyl_points_2d_h[2]

    debug('cyl_points_2d')
    debug(cyl_points_2d)

    if 1:
        if 1:
            x0 = 0
            y0 = 0

            gl_viewport_args = x0, y0, img_width, img_height
            proj = convert_hz_intrinsic_to_opengl_projection(calib['intrinsic'],
                                                             x0,y0,img_width,img_height, 0.1, 1000.0,
                                                             window_coords=window_coords)

        clip = np.dot(proj,cyl_points_3d_eye_opengl_h)
        debug('clip')
        debug(clip)
        ndc = clip[:3,:]/clip[3,:]
        debug('ndc')
        debug(ndc)
        window_x = gl_viewport_args[2]/2.0 * (ndc[0,:] + 1)+int(gl_viewport_args[0])
        window_y = gl_viewport_args[3]/2.0 * (ndc[1,:] + 1)+int(gl_viewport_args[1])
        window_gl = np.vstack((window_x,window_y))
        debug('window_gl')
        debug(window_gl)

    if 1:
        if window_coords=='y down':
            luminance = luminance[::-1]
            yc = img_height-cyl_points_2d[1,:]
        else:
            assert window_coords=='y up'
            yc = cyl_points_2d[1,:]
        plt.imshow(luminance, interpolation='nearest', origin='lower', cmap=matplotlib.cm.gray)
        plt.plot( cyl_points_2d[0,:], yc, 'b+', ms=15.0 )
        plt.plot( window_x, window_y, 'r.' )
        plt.show()

if __name__=='__main__':
    main()

