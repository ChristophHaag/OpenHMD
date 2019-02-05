#ifndef SIMPLERENDERER_H
#define SIMPLERENDERER_H

#include <SDL.h>

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1

#ifndef __APPLE__
#include <GL/glew.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#include "../../include/openhmd.h"

typedef struct SimpleRenderer
{
	int w, h;
	SDL_Window* window;
	SDL_GLContext glcontext;
	SDL_Window* applicationwindow;
	SDL_GLContext applicationglcontext;
	int is_fullscreen;
	ohmd_device *device;

	float ipd;
	float viewport_scale[2];
	float distortion_coeffs[4];
	float aberr_scale[3];
	float sep;
	float left_lens_center[2];
	float right_lens_center[2];
	float warp_scale;
	float warp_adj;
	
	GLuint distortionVAO[1];
	float quadvertices[6];
	GLuint distortionshader;

} SimpleRenderer;

SimpleRenderer *
openhmd_create_simple_renderer (ohmd_device *device, SDL_GLContext appcontext, SDL_Window *appwindow);

void
openhmd_render_companion(SimpleRenderer *renderer, GLuint tex);

void
openhmd_render_textures (SimpleRenderer *renderer, GLuint left, GLuint right);

#endif
