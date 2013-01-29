from __future__ import division
import scipy.linalg
import numpy as np

def my_rq(M):
    """RQ decomposition, ensures diagonal of R is positive"""
    R,K = scipy.linalg.rq(M)
    n = R.shape[0]
    for i in range(n):
        if R[i,i]<0:
            R[:,i] = -R[:,i]
            K[i,:] = -K[i,:]
    return R,K

def pmat2cam_center(P):
    """

    See Hartley & Zisserman (2003) p. 163
    """
    assert P.shape == (3,4)
    determinant = np.linalg.det

    # camera center
    X = determinant( [ P[:,1], P[:,2], P[:,3] ] )
    Y = -determinant( [ P[:,0], P[:,2], P[:,3] ] )
    Z = determinant( [ P[:,0], P[:,1], P[:,3] ] )
    T = -determinant( [ P[:,0], P[:,1], P[:,2] ] )

    C_ = np.transpose(np.array( [[ X/T, Y/T, Z/T ]] ))
    return C_

def decompose(pmat):
    M = pmat[:,:3]
    K,R = my_rq(M)
    K = K/K[2,2] # normalize intrinsic parameter matrix
    C_ = pmat2cam_center(pmat)
    t = np.dot( -R, C_)
    Rt = np.hstack((R, t ))

    return dict( intrinsic=K,
                 rotation=R,
                 cam_center=C_,
                 t=t,
                 extrinsic=Rt,
                 )

def generate_cyl_points(homog=False,n_segs=30):
    r = 0.5
    h = 1.0
    z0 = 0.0
    theta0 = 0

    theta = np.linspace( 0, 2*np.pi, num=n_segs, endpoint=False)+theta0
    z = np.linspace( 0, h, num=2)+z0
    points = []
    for zi in z:
        xx = r*np.cos(theta)
        yy = r*np.sin(theta)
        zz = zi*np.ones_like(xx)
        if homog:
            ww = np.ones_like(xx)
            points.extend([(xx[i], yy[i], zz[i],ww[i]) for i in range(theta.shape[0])])
        else:
            points.extend([(xx[i], yy[i], zz[i]) for i in range(theta.shape[0])])
    points = np.array(points).T
    return points

def convert_hz_intrinsic_to_opengl_projection(K,x0,y0,width,height,znear,zfar, window_coords=None):
    znear = float(znear)
    zfar = float(zfar)
    depth = zfar - znear
    q = -(zfar + znear) / depth
    qn = -2 * (zfar * znear) / depth

    if window_coords=='y up':
        proj = np.array([[ 2*K[0,0]/width, -2*K[0,1]/width, (-2*K[0,2]+width+2*x0)/width, 0 ],
                         [  0,             -2*K[1,1]/height,(-2*K[1,2]+height+2*y0)/height, 0],
                         [0,0,q,qn],  # This row is standard glPerspective and sets near and far planes.
                         [0,0,-1,0]]) # This row is also standard glPerspective.
    else:
        assert window_coords=='y down'
        proj = np.array([[ 2*K[0,0]/width, -2*K[0,1]/width, (-2*K[0,2]+width+2*x0)/width, 0 ],
                         [  0,              2*K[1,1]/height,( 2*K[1,2]-height+2*y0)/height, 0],
                         [0,0,q,qn],  # This row is standard glPerspective and sets near and far planes.
                         [0,0,-1,0]]) # This row is also standard glPerspective.
    return proj

def get_gluLookAt(pmat):
    calib = decompose(pmat)
    R = calib['rotation']
    Rinv = np.linalg.pinv(R)
    up_camera = np.array([[0,-1.0,0.0]]).T
    upVector3D = np.dot( Rinv, up_camera )

    ahead_camera = np.array([[0,0,1.0]]).T
    forward = np.dot( Rinv, ahead_camera )

    center3D = calib['cam_center'] + forward

    result = dict( eye=calib['cam_center'][:,0],
                   center=center3D[:,0],
                   up=upVector3D[:,0],
                   )
    result = dict([ (k, map(float, v)) for (k,v) in result.iteritems()])
    all_args = []
    for v in (result['eye'], result['center'], result['up']):
        for i in range(3):
            all_args.append( v[i] )
    result['all_args']=all_args
    return result
