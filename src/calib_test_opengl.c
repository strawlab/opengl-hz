/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <GL/glut.h>

#define PI 3.14159

// global
unsigned int CYL;

void set_matrix(float* m,
                float M00, float M01, float M02, float M03,
                float M10, float M11, float M12, float M13,
                float M20, float M21, float M22, float M23,
                float M30, float M31, float M32, float M33) {
    m[0] = M00;
    m[1] = M10;
    m[2] = M20;
    m[3] = M30;

    m[4] = M01;
    m[5] = M11;
    m[6] = M21;
    m[7] = M31;

    m[8] = M02;
    m[9] = M12;
    m[10]= M22;
    m[11]= M32;

    m[12]= M03;
    m[13]= M13;
    m[14]= M23;
    m[15]= M33;
}

void on_draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPointSize(5.0);
    glCallList(CYL);
    glColor3f(1.0, 1.0, 1.0);

    glutSwapBuffers();
}

void on_resize(int width, int height) {
    int x0, y0;
    float* m;

    x0 = 0;
    y0 = 0;

    glViewport(x0, y0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //  gluPerspective( 80.0, 1.0, 0.1, 10.0 );
    m = malloc(16*sizeof(float));
    set_matrix( m,
                1.60744584,  0.01951438,  0.05250012,  0.,
                0.,          2.40880443,  0.07234515,  0.   ,
                0.,          0.,         -1.00020002, -0.20002,
                0.,          0.,         -1.,          0.);
    glLoadMatrixf(m);
    free(m);
}

void assign_vert3f( float* base_ptr, int idx, int len, float a, float b, float c ) {
    base_ptr[idx*3+0] = a;
    base_ptr[idx*3+1] = b;
    base_ptr[idx*3+2] = c;
}

void make_cyl_data( int n_segs, float* vertices, float* colors, unsigned int* indices) {
    int i;

    double r, h, z0, theta0, theta_inc, theta, zi, xx, yy, zz;
    double xo, yo, zo;

    xo=0;
    yo=0;
    zo=0;

    r=0.5;
    h = 1.0;
    z0 = 0.0;
    theta0 = 0.0;
    theta_inc = 2*PI/(double)n_segs;

    for (i=0; i<n_segs; i++) {
        theta = i*theta_inc+theta0;
        zi = z0;
        xx = r*cos(theta); yy = r*sin(theta); zz = zi;
        assign_vert3f(vertices, i, n_segs*2, xx+xo, yy+yo, zz+zo);
        assign_vert3f(colors, i, n_segs*2, 0.0, 1.0, 0.0);
        indices[i] = i;
    }

    for (i=0; i<n_segs; i++) {
        theta = i*theta_inc+theta0;
        zi = z0+h;
        xx = r*cos(theta); yy = r*sin(theta); zz = zi;
        assign_vert3f(vertices, i+n_segs, n_segs*2, xx+xo, yy+yo, zz+zo);
        assign_vert3f(colors, i+n_segs, n_segs*2, 0.0, 1.0, 0.0);
        indices[i+n_segs] = i+n_segs;
    }
}

unsigned int PointCylinder() {
    float* vertices;
    float* colors;
    unsigned int* indices;
    int len_indices, n_segs;

    unsigned int result;

    // PointCylinder
    n_segs = 30;

    vertices=NULL;
    colors=NULL;
    indices=NULL;

    vertices = malloc(3*n_segs*2*sizeof(float));
    colors = malloc(3*n_segs*2*sizeof(float));
    indices = malloc(n_segs*2*sizeof(unsigned int));
    len_indices=n_segs*2;

    make_cyl_data( n_segs, vertices, colors, indices);

    result = 0;
    result = glGenLists(1);
    glNewList(result, GL_COMPILE);

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(3, GL_FLOAT, 0, colors);
    glDrawElements(GL_POINTS, len_indices, GL_UNSIGNED_INT, indices);
    glPopClientAttrib();

    glEndList();
    return result;
}

int main(int argc, char* argv[]) {
    int width, height;

    width=752;
    height = 480;

    glutInit(&argc, argv);
    glutInitWindowSize(width,height);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_ACCUM | GLUT_DOUBLE);
    (void) glutCreateWindow("calib_test_opengl");

    CYL = PointCylinder();

    if (1) {
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        gluLookAt(-0.70847115249281878, -1.418418122404193, 1.3039421809923302, // eye
                  -0.28002777111508598, -0.6477648044248473, 0.83221160911783199, //center
                  -0.19730308528412435, -0.4296835651435702, -0.88116032955579493); // up
    }
    glutDisplayFunc(on_draw);
    glutReshapeFunc(on_resize);
    glutMainLoop();
    return 0;
}
