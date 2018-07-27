#ifndef SIMPLECOMPOSITOR
#define SIMPLECOMPOSITOR

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1

#include <GL/gl.h>
#include <GL/glx.h>

//TODO put in proper struct somewhere and pass around pointer to it as state
GLuint left_color_tex = 0, left_depth_tex = 0, left_fbo = 0;
GLuint right_color_tex = 0, right_depth_tex = 0, right_fbo = 0;
GLXContext appctx;
GLuint shaderProgramID;
float left_lens_center[2];
float right_lens_center[2];
GLuint VAO;
Display *display;
Window win;
int eye_w;
int eye_h;
int hmd_w, hmd_h;

int initCompositor(int hmd_index);
void submitFrame();

#endif
