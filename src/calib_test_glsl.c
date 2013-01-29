/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <GL/glut.h>

#define PI 3.14159

// globals
unsigned int CYL;
GLint program = 0;
GLint projection_matrix_location, modelview_matrix_location;
float proj[16];
float mv[16];

/* shader reader */
/* creates null terminated string from file */

char* readShaderSource(const char* shaderFile)
/* From http://www.cs.unm.edu/~angel/BOOK/INTERACTIVE_COMPUTER_GRAPHICS/FOURTH_EDITION/PROGRAMS/GLSL_example */
{
    struct stat statBuf;
    FILE* fp = fopen(shaderFile, "r");

    if (!fp)
    {
        fprintf(stderr,"Could not open file: %s\n", shaderFile);
        exit(EXIT_FAILURE);
    }

    char* buf;

    stat(shaderFile, &statBuf);
    buf = (char*) malloc(statBuf.st_size + 1 * sizeof(char));
    fread(buf, 1, statBuf.st_size, fp);
    buf[statBuf.st_size] = '\0';
    fclose(fp);
    return buf;
}

/* error printing function */

static void checkError(GLint status, const char *msg, GLuint shader)
/* From http://www.cs.unm.edu/~angel/BOOK/INTERACTIVE_COMPUTER_GRAPHICS/FOURTH_EDITION/PROGRAMS/GLSL_example */
{
    GLsizei maxLength;
    GLchar *infoLog;
    GLsizei length;

    if (status==GL_FALSE)
    {
        printf("%s\n", msg);

        maxLength=1024;
        infoLog = malloc( sizeof(GLchar)*maxLength );
        assert( infoLog != 0 );
        glGetShaderInfoLog(shader, maxLength,
                           &length, infoLog);
        fprintf(stderr, "%s\n", infoLog );
        free(infoLog);

        exit(EXIT_FAILURE);
    }
}

static void initShader(const GLchar* vShaderFile, const GLchar* fShaderFile)
/* From http://www.cs.unm.edu/~angel/BOOK/INTERACTIVE_COMPUTER_GRAPHICS/FOURTH_EDITION/PROGRAMS/GLSL_example */
{
    GLint vShader = 0;
    GLint fShader = 0;
    GLint status = 0;

    /* read shader files */

    GLchar* vSource = readShaderSource(vShaderFile);
    GLchar* fSource = readShaderSource(fShaderFile);

    /* create program and shader objects */

    vShader = glCreateShader(GL_VERTEX_SHADER);
    fShader = glCreateShader(GL_FRAGMENT_SHADER);
    program = glCreateProgram();

    /* attach shaders to the program object */

    glAttachShader(program, vShader);
    glAttachShader(program, fShader);

    /* read shaders */

    glShaderSource(vShader, 1, &vSource, NULL);
    glShaderSource(fShader, 1, &fSource, NULL);

    /* compile shaders */

    glCompileShader(vShader);
    glCompileShader(fShader);

    /* error check */

    glGetShaderiv(vShader, GL_COMPILE_STATUS, &status);
    checkError(status, "Failed to compile the vertex shader.", vShader);

    glGetShaderiv(fShader, GL_COMPILE_STATUS, &status);
    checkError(status, "Failed to compile the fragment shader.", fShader);

    /* link */

    glLinkProgram(program);
    glGetShaderiv(program, GL_LINK_STATUS, &status);
    checkError(status, "Failed to link the shader program object.", 0);

    /* use program object */

    glUseProgram(program);

    /* set up uniform parameter */

    projection_matrix_location = glGetUniformLocation(program, "projection_matrix");
    modelview_matrix_location = glGetUniformLocation(program, "modelview_matrix");

    /* Free allocated memory */
    free(vSource);
    free(fSource);

}

/* --------------------------------------------------- */

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
    int i,j;

    x0 = 0;
    y0 = 0;

    glViewport(x0, y0, width, height);


    //  gluPerspective( 80.0, 1.0, 0.1, 10.0 );
    set_matrix( proj,
                1.60744584,  0.01951438,  0.05250012,  0.,
                0.,          2.40880443,  0.07234515,  0.   ,
                0.,          0.,         -1.00020002, -0.20002,
                0.,          0.,         -1.,          0.);

    glUniformMatrix4fv(projection_matrix_location, 1, GL_FALSE, proj);

    printf( "PROJECTION\n" );

    for (i=0; i<4; i++) {
        for (j=0; j<4; j++) {
            printf( "%f ",proj[i*4+j]);
        }
        printf( "\n" );
    }

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
    indices = malloc(3*n_segs*2*sizeof(unsigned int));
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
    float modelview[16];

    width=752;
    height = 480;

    glutInit(&argc, argv);
    glutInitWindowSize(width,height);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_ACCUM | GLUT_DOUBLE);
    (void) glutCreateWindow("calib_test_glsl");

    CYL = PointCylinder();
    initShader("glsl.vert","glsl.frag");

    if (1) {
        set_matrix( mv,
                    -0.881764, 0.470601, -0.032043, 0.084587,
                    -0.197303, -0.429684, -0.881160, 0.399728,
                    -0.428443, -0.770653, 0.471731, -2.011758,
                    0.000000, 0.000000, 0.000000, 1.000000);
        glUniformMatrix4fv(modelview_matrix_location, 1, GL_FALSE, mv);
    }
    glutDisplayFunc(on_draw);
    glutReshapeFunc(on_resize);
    glutMainLoop();
    return 0;
}
