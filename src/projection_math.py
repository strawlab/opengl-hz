from __future__ import division
import sympy
from sympy import Symbol, Matrix, sqrt, latex, lambdify
from sympy.utilities.lambdify import lambdastr
from sympy.solvers import solve
import numpy as np


def eye(sz):
    rows = []
    for i in range(sz):
        row = []
        for j in range(sz):
            if i==j:
                row.append( 1 )
            else:
                row.append( 0 )
        rows.append(row)
    return Matrix(rows)

for window_coords in ['y up','y down']:

    width = Symbol('width')
    height = Symbol('height')

    # We start with "eye coordinates" (the coordinates of objects in
    # the camera's coordinate system).

    x_e = Symbol('x_e')
    y_e = Symbol('y_e')
    z_e = Symbol('z_e')
    w_e = Symbol('w_e')

    eye_hz = Matrix([[x_e],
                     [y_e],
                     [z_e],
                     [w_e]])

    # HZ and OpenGL have different coordinate systems. We deal with
    # that in our eye coordinates. This way, an object viewed with a
    # standard HZ camera with +Y going down at looking at +Z will have
    # different eye coordinates as an object in OpenGL, but it will
    # "look" the same in the sense that when the camera is pointed at
    # the object, it will be up on the image plane will be up in both
    # cases. ("Pointing" and "up" meaning different things, as
    # specified by the coordinate system.)

    coord_xform = eye(4)
    coord_xform[1,1]=-1 # flip Y coordinate (HZ camera has +y going down, GL has +y going up)
    coord_xform[2,2]=-1 # flip Z coordinate (HZ camera looks at +z, GL looks at -z)
    eye_gl = coord_xform*eye_hz

    # Create a sympy model of the OpenGL pipeline.

    if 1:
        # glp = the GL Projection matrix
        glp00 = Symbol('glp00')
        glp01 = Symbol('glp01')
        glp02 = Symbol('glp02')

        glp11 = Symbol('glp11')
        glp12 = Symbol('glp12')

        znear = Symbol('znear')
        zfar  = Symbol('zfar')

        depth = zfar - znear
        q = -(zfar + znear) / depth
        qn = -2 * (zfar * znear) / depth

        # We define the matrix with zeros where we don't need
        # values. This simplification allows us to solve analytically
        # for the remaining values.

        glp = Matrix([[glp00, glp01, glp02,    0 ],
                      [ 0,    glp11, glp12,    0 ],
                      [ 0,      0,    q,      qn ],  # This row is standard glPerspective and sets near and far planes.
                      [ 0,      0,   -1,       0 ]]) # This row is also standard glPerspective.

    if 1:
        # Take the eye coordinates and create clip coordinates from
        # them.
        clip = glp*eye_gl

    if 1:
        # Now take the clip coordinates and create normalized device
        # coordinates.
        NDC = Matrix([[ clip[0,0] / clip[3,0] ],
                      [ clip[1,0] / clip[3,0] ],
                      [ clip[2,0] / clip[3,0] ]])
    if 1:
        # Finally, model the glViewport transformation.
        if 1:
            x0 = Symbol('x0')
            y0 = Symbol('y0')
        else:
            x0 = 0
            y0 = 0
        window_gl = Matrix([[ (NDC[0,0] + 1)*(width/2)+x0 ],
                         [ (NDC[1,0] + 1)*(height/2)+y0 ],
                         # TODO: there must be something for Z
                         ])

    # The HZ pipeline is much simpler - a single matrix
    # multiplication.

    if 1:
        # intrinsic matrix (upper triangular with last entry 1)
        K00 = Symbol('K00')
        K01 = Symbol('K01')
        K02 = Symbol('K02')

        K11 = Symbol('K11')
        K12 = Symbol('K12')

        K = Matrix([[K00, K01, K02],
                    [  0, K11, K12],
                    [  0,   0,   1]])
    if 1:
        eye3=eye_hz[:3,0]
        window_hz_h = K*eye3
    if window_coords=='y up':
        window_hz = Matrix([[ window_hz_h[0,0]/window_hz_h[2,0] ],
                            [ window_hz_h[1,0]/window_hz_h[2,0] ]])
    else:
        assert window_coords=='y down'
        window_hz = Matrix([[ window_hz_h[0,0]/window_hz_h[2,0] ],
                            [ height - window_hz_h[1,0]/window_hz_h[2,0] ]])


    # Now, using all the above, solve for the entries in the GL
    # projection matrix. (If I knew more sympy, this could doubtless
    # be done in a single solve step with multiple equations instead
    # of the solve and substitute approach I'm taking here.)

    # sympy solves expressions by creating an equation where the left
    # hand side is your expression and the right hand size is zero.
    # The expression to solve for "window_gl[0,0] == window_hz[0,0]"
    # is thus:
    expr = window_gl[0,0] - window_hz[0,0]

    e2 = expr.subs( {x_e: 0, y_e: 0, z_e: 1})
    glp02_expr = solve(e2, glp02)
    assert len(glp02_expr)==1
    glp02_expr = glp02_expr[0]

    e2 = expr.subs( {x_e: 0, y_e: 1, z_e: 1, glp02: glp02_expr})
    glp01_expr = solve(e2, glp01)
    assert len(glp01_expr)==1
    glp01_expr = glp01_expr[0]

    e2 = expr.subs( {x_e: 1, y_e: 0, z_e: 1, glp01: glp01_expr, glp02: glp02_expr})
    glp00_expr = solve(e2, glp00)
    assert len(glp00_expr)==1
    glp00_expr = glp00_expr[0]



    expr = window_gl[1,0] - window_hz[1,0]

    e2 = expr.subs( {x_e: 0, y_e: 0, z_e: 1})
    glp12_expr = solve(e2, glp12)
    assert len(glp12_expr)==1
    glp12_expr = glp12_expr[0]

    e2 = expr.subs( {x_e: 0, y_e: 1, z_e: 1, glp12: glp12_expr})
    glp11_expr = solve(e2, glp11)
    assert len(glp11_expr)==1
    glp11_expr = glp11_expr[0]

    GLP = Matrix([[ glp00_expr, glp01_expr, glp02_expr, 0],
                  [ 0,          glp11_expr, glp12_expr, 0],
                  [ 0,      0,    q,      qn ],  # This row is standard glPerspective and sets near and far planes.
                  [ 0,      0,   -1,       0 ]]) # This row is also standard glPerspective.
    print 'window_coords=',repr(window_coords)
    print GLP
