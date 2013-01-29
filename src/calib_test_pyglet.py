#!/usr/bin/env python

# stdlib imports
import os
from contextlib import contextmanager

#other imports
import numpy as np
import pyglet
import pyglet.gl as gl
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
        gl.glPopClientAttrib()

        gl.glEndList()

    def draw(self):
        gl.glPointSize(5.0)
        gl.glCallList(self.list)
        gl.glColor3f(1.0, 1.0, 1.0)

class MyAppWindow(pyglet.window.Window):
    def __init__(self,image_fname=None,pmat=None,window_coords=None,**kwargs):
        if window_coords is None:
            # set default value
            window_coords = 'y down'
        super(MyAppWindow, self).__init__(**kwargs)

        self.calib = decompose(pmat)
        self.window_coords = window_coords
        self.img = pyglet.image.load(image_fname).get_texture(rectangle=True)
        if self.window_coords=='y up':
            self.img = self.img.get_transform(flip_y=True)
        self.img.anchor_x = self.img.anchor_y = 0
        self.width = self.img.width
        self.height = self.img.height

        checks = pyglet.image.create(32, 32, pyglet.image.CheckerImagePattern())
        self.background = pyglet.image.TileableTexture.create_for_image(checks)

        # Enable alpha blending, required for image.blit.
        gl.glEnable(gl.GL_BLEND)
        gl.glBlendFunc(gl.GL_SRC_ALPHA, gl.GL_ONE_MINUS_SRC_ALPHA)

        self.cyl = PointCylinder()

        # set modelview matrix to camera extrinsic parameters
        if 0:
            # do it directly
            e = np.vstack((self.calib['extrinsic'],[[0,0,0,1]])) # These HZ eye coords have +Z in front of camera.
            coord_xform = np.eye(4)
            coord_xform[1,1]=-1 # flip Y coordinate in eye space (OpenGL has +Y as up, HZ has -Y)
            coord_xform[2,2]=-1 # flip Z coordinate in eye space (OpenGL has -Z in front of camera, HZ has +Z)
            e2 = np.dot( coord_xform, e)
            extrinsic = map(float,e2.T.flat)
            extrinsic = (gl.GLfloat * 16)(*extrinsic)
            gl.glMatrixMode(gl.GL_MODELVIEW)
            gl.glLoadMatrixf(extrinsic)
        else:
            # compose view matrix
            r = get_gluLookAt(pmat)
            gl.glMatrixMode(gl.GL_MODELVIEW)
            gl.glLoadIdentity()
            gl.gluLookAt( *r['all_args'] )
        gl.glDisable(gl.GL_DEPTH_TEST)

    def on_draw(self):
        self.clear()
        with self.window_coordinates_ydown():
            self.background.blit_tiled(0, 0, 0, self.width, self.height)
            self.img.blit(0,0,0)
        self.cyl.draw()

    @contextmanager
    def window_coordinates_ydown(self):

        # These are screen coords. Y increases downward, with 0 at top
        # of initial window. (Standard OpenGL has 0 at bottom and Y
        # increasing upward.)

        gl.glPushMatrix()
        gl.glLoadIdentity()
        gl.glMatrixMode(gl.GL_PROJECTION)
        gl.glPushMatrix()
        gl.glLoadIdentity()
        gl.glOrtho(0, self.width, 0, self.height, -1, 1)
        gl.glViewport(0, 0, self.width, self.height)

        yield # perform drawing

        gl.glViewport(*self.gl_viewport_args)
        gl.glPopMatrix()
        gl.glMatrixMode(gl.GL_MODELVIEW)
        gl.glPopMatrix()

    def on_resize(self, width, height):
        # load HZ matrix into OpenGL equivalent
        x0 = 0
        y0 = 0

        self.gl_viewport_args = x0, y0, self.img.width, self.img.height
        gl.glViewport(*self.gl_viewport_args)
        znear, zfar = .1, 1000.
        proj = convert_hz_intrinsic_to_opengl_projection(self.calib['intrinsic'],
                                                         x0,y0,self.img.width,self.img.height, znear, zfar,
                                                         window_coords=self.window_coords)

        m = map(float,proj.T.flat)
        m = (gl.GLfloat * 16)(*m)

        gl.glMatrixMode(gl.GL_PROJECTION)
        gl.glLoadMatrixf(m)
        gl.glMatrixMode(gl.GL_MODELVIEW)
        return pyglet.event.EVENT_HANDLED

def main():
    src_dir = os.path.split(os.path.abspath(__file__))[0]
    data_dir = os.path.join(src_dir,'..','data')
    pmat = np.loadtxt( os.path.join(data_dir, 'cameramatrix.txt') )
    image_fname = os.path.join(data_dir, 'luminance.png' )
    window = MyAppWindow(pmat=pmat,image_fname=image_fname,resizable=True)
    pyglet.app.run()

if __name__=='__main__':
    main()

