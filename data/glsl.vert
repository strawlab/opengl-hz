/* -*- Mode: C -*- */
#version 120

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

void main(void)
{
  vec4 vert_world = gl_Vertex;
  vec4 vert_eye = modelview_matrix * vert_world;
  vec4 vert_clip = projection_matrix * vert_eye;
  gl_Position	= vert_clip;
}

