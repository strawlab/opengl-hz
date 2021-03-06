# opengl-hz

### multiple implementations of the OpenGL geometry pipeline for learning

This package includes and uses multiple implementations of the OpenGL
geometry pipeline. Its purpose is to allow the curious coder to play
around with the various approaches to decipher what OpenGL is doing
with your coordinates.

For example, this code was written when I needed to compute an OpenGL
projection matrix from a general camera calibration. See the blog post
[Augmented Reality with
OpenGL](http://strawlab.org/2011/11/05/augmented-reality-with-OpenGL/).

Implementations:

 * Python, no OpenGL (numpy for geometry, matplotlib for drawning) `src/calib_test_numpy.py`.
 * Python (pyglet for OpenGL and GUI) `src/calib_test_pyglet.py`.
 * Python (PyOpenGL for OpenGL and GUI) `src/calib_test_pyglet.py`.
 * C (OpenGL, GLUT for GUI) `src/calib_test_opengl.c`.
 * C++ (OpenSceneGraph) `src/calib_test_osg.cpp`.
 * C/GLSL (OpenGL, GLUT for GUI) `src/calib_test_glsl.c`, `glsl.vert`, `glsl.frag`.

All programs should be run from the `data/` directory so they find
the required files.

## Quickstart instructions

So far only linux has been tested.

### Build on linux

This is tested on Ubuntu 12.04:

    mkdir build
    cd build
    cmake .. && make

### Run on linux

Now change to the `data/` directory and run your just-build
executable:

    cd ../data
    ../build/bin/calib_test_osg

Python scripts aren't copied into `build/bin/`, so run from the
`src/` directory:

    python ../src/calib_test_pyglet.py
