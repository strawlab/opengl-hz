#!/usr/bin/env python

# stdlib imports
import os
from contextlib import contextmanager

#other imports
import numpy as np
import OpenGL.GL as gl
import OpenGL.GLU as glu
import OpenGL.GLUT as glut
from calib_test_utils import generate_cyl_points, decompose, \
     convert_hz_intrinsic_to_opengl_projection, get_gluLookAt

R2D = 180.0/np.pi

class PointCylinder(object):
    def __init__(self):
        pts = generate_cyl_points().T

        colors = np.zeros_like(pts)
        colors[:,1]=1.0 # all green

        n_pts = len(pts)
        pts = map(float,pts.flat) # convert to flat list of floats
        colors = map(float, colors.flat)

        # Create ctypes arrays of the lists
        vertices = (gl.GLfloat * (n_pts*3))(*pts)
        colors = (gl.GLfloat * (n_pts*3))(*colors)

        # Create a list of triangle indices.
        indices = range(n_pts)
        indices = (gl.GLuint * n_pts)(*indices)

        # Compile a display list
        self.list = gl.glGenLists(1)
        gl.glNewList(self.list, gl.GL_COMPILE)

        gl.glPushClientAttrib(gl.GL_CLIENT_VERTEX_ARRAY_BIT)
        gl.glEnableClientState(gl.GL_VERTEX_ARRAY)
        gl.glVertexPointer(3, gl.GL_FLOAT, 0, vertices)
        gl.glEnableClientState(gl.GL_COLOR_ARRAY)
        gl.glColorPointer(3, gl.GL_FLOAT, 0, colors)
        gl.glDrawElements(gl.GL_POINTS, len(indices), gl.GL_UNSIGNED_INT, indices)
        #gl.glDrawElements(gl.GL_QUAD_STRIP, len(indices), gl.GL_UNSIGNED_INT, indices)
        gl.glPopClientAttrib()

        gl.glEndList()

    def draw(self):
        gl.glPointSize(5.0)
        gl.glCallList(self.list)
        gl.glColor3f(1.0, 1.0, 1.0)

def main():
    global calib, cyl

    src_dir = os.path.split(os.path.abspath(__file__))[0]
    data_dir = os.path.join(src_dir,'..','data')
    pmat = np.loadtxt( os.path.join(data_dir, 'cameramatrix.txt') )
    calib = decompose(pmat)
    width = 752
    height = 480

    glut.glutInit()
    glut.glutInitWindowSize(width,height)
    glut.glutInitDisplayMode(glut.GLUT_RGBA | glut.GLUT_DEPTH | glut.GLUT_ACCUM | glut.GLUT_DOUBLE)
    glut.glutCreateWindow("calib_test_pyopengl");

    cyl = PointCylinder()
    if 1:
        # compose view matrix
        r = get_gluLookAt(pmat)
        gl.glMatrixMode(gl.GL_MODELVIEW)
        gl.glLoadIdentity()
        glu.gluLookAt( *r['all_args'] )
    gl.glDisable(gl.GL_DEPTH_TEST)
    glut.glutDisplayFunc(on_draw)
    on_resize(width,height)
    glut.glutMainLoop()

def on_draw():
    global cyl
    gl.glClear(gl.GL_COLOR_BUFFER_BIT | gl.GL_DEPTH_BUFFER_BIT);
    cyl.draw()
    glut.glutSwapBuffers()

def on_resize(width,height):
    global calib
    x0 = 0
    y0 = 0

    gl.glViewport(x0, y0, width, height)
    znear, zfar = .1, 1000.
    proj = convert_hz_intrinsic_to_opengl_projection(calib['intrinsic'],
                                                     x0,y0,width,height, znear, zfar,
                                                     window_coords='y down')
    m = map(float,proj.T.flat)
    m = (gl.GLfloat * 16)(*m)

    gl.glMatrixMode(gl.GL_PROJECTION)
    gl.glLoadMatrixf(m)
    gl.glMatrixMode(gl.GL_MODELVIEW)

if __name__=='__main__':
    main()

