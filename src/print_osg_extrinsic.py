#!/usr/bin/env python

import os
import numpy as np
from calib_test_utils import get_gluLookAt

data_dir = os.path.split(os.path.abspath(__file__))[0]
pmat = np.loadtxt( os.path.join(data_dir, 'cameramatrix.txt') )
r = get_gluLookAt(pmat)

for n in ['eye','center','up']:
    print 'osg::Vec3 %s = osg::Vec3(%s,%s,%s);'%tuple([n]+r[n])

