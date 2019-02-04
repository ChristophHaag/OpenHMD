/*
 * OpenHMD - Free and Open Source API and drivers for immersive technology.
 * Copyright (C) 2013 Fredrik Hultin.
 * Copyright (C) 2013 Jakob Bornecrantz.
 * Distributed under the Boost 1.0 licence, see LICENSE for full text.
 */

/* OpenGL Test - Main Implementation */

#include <openhmd.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "gl.h"
#define MATH_3D_IMPLEMENTATION
#include "math_3d.h"

#include "simplerenderer.h"

#define degreesToRadians(angleDegrees) ((angleDegrees) * M_PI / 180.0)
#define radiansToDegrees(angleRadians) ((angleRadians) * 180.0 / M_PI)

#define OVERSAMPLE_SCALE 2.0

void GLAPIENTRY
gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                  GLsizei length, const GLchar* message, const void* userParam)
{
	fprintf(stderr, "GL DEBUG CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
	        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
	        type, severity, message);
}

float randf()
{
	return (float)rand() / (float)RAND_MAX;
}

void draw_crosshairs(float len, float cx, float cy)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glBegin(GL_LINES);
	float l = len/2.0f;
	glVertex3f(cx - l, cy, 0.0);
	glVertex3f(cx + l, cy, 0.0);
	glVertex3f(cx, cy - l, 0.0);
	glVertex3f(cx, cy + l, 0.0);
	glEnd();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

static inline void
print_matrix(float m[])
{
    printf("[[%0.4f, %0.4f, %0.4f, %0.4f],\n"
            "[%0.4f, %0.4f, %0.4f, %0.4f],\n"
            "[%0.4f, %0.4f, %0.4f, %0.4f],\n"
            "[%0.4f, %0.4f, %0.4f, %0.4f]]\n",
            m[0], m[4], m[8], m[12],
            m[1], m[5], m[9], m[13],
            m[2], m[6], m[10], m[14],
            m[3], m[7], m[11], m[15]);
}

// cubes arranged in a circle around the user, 20° between them. 360°/20° = 18 cubes
mat4_t cube_modelmatrix[18];
vec3_t cube_colors[18];
float cube_alpha[18];

void gen__cubes()
{
	for(int i = 0; i < 18; i ++) {
		cube_modelmatrix[i] = m4_identity();
		cube_modelmatrix[i] = m4_mul(cube_modelmatrix[i], m4_rotation(degreesToRadians(i * 20), vec3(0, 1, 0)));
		cube_modelmatrix[i] = m4_mul(cube_modelmatrix[i], m4_translation(vec3(0, 1.8, -5)));
		cube_modelmatrix[i] = m4_mul(cube_modelmatrix[i], m4_rotation(degreesToRadians(randf() * 360), vec3(randf(), randf(), randf())));

		cube_colors[i] = vec3(randf(), randf(), randf());
		cube_alpha[i] = randf();
	}
}

void draw_cubes(GLuint shader)
{
	int modelLoc = glGetUniformLocation(shader, "model");
	int colorLoc = glGetUniformLocation(shader, "uniformColor");
	for(int i = 0; i < 18; i ++) {
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*) cube_modelmatrix[i].m);
		glUniform4f(colorLoc, cube_colors[i].x, cube_colors[i].y, cube_colors[i].z, cube_alpha[i]);
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// floor is 10x10m, 0.1m thick
	mat4_t floor = m4_identity();
	floor = m4_mul(floor, m4_scaling(vec3(10, 0.1, 10)));
	// we could move the floor to -1.8m height if the HMD tracker sits at zero
	// floor = m4_mul(floor, m4_translation(vec3(0, -1.8, 0)));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*) floor.m);
	glUniform4f(colorLoc, 0, .4f, .25f, .9f);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void draw_controllers(GLuint shader, ohmd_device *lc, ohmd_device *rc)
{
	int modelLoc = glGetUniformLocation(shader, "model");
	int colorLoc = glGetUniformLocation(shader, "uniformColor");

	mat4_t lcmodel;
	ohmd_device_getf(lc, OHMD_GL_MODEL_MATRIX, (float*) lcmodel.m);
	lcmodel = m4_mul(lcmodel, m4_scaling(vec3(0.03, 0.03, 0.1)));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*) lcmodel.m);
	glUniform4f(colorLoc, 1.0, 0.0, 0.0, 1.0);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	mat4_t rcmodel;
	ohmd_device_getf(rc, OHMD_GL_MODEL_MATRIX, (float*) rcmodel.m);
	rcmodel = m4_mul(rcmodel, m4_scaling(vec3(0.03, 0.03, 0.1)));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, (float*) rcmodel.m);
	glUniform4f(colorLoc, 0.0, 1.0, 0.0, 1.0);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

int main(int argc, char** argv)
{
	int hmddev = -1;
	int lcontrollerdev = -1;
	int rcontrollerdev = -1;

	/* User can set devices to use with flags. There is no validation that -lc is actually a left controller*/
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-hmd") == 0 && i < argc - 1) {
			hmddev = strtol(argv[i+1], NULL, 10);
			printf("Using HMD device %d\n", hmddev);
		}
		if (strcmp(argv[i], "-lc") == 0 && i < argc - 1) {
			lcontrollerdev = strtol(argv[i+1], NULL, 10);
			printf("Using left controller device %d\n", lcontrollerdev);
		}
		if (strcmp(argv[i], "-rc") == 0 && i < argc - 1) {
			rcontrollerdev = strtol(argv[i+1], NULL, 10);
			printf("Using right controller device %d\n", rcontrollerdev);
		}
	}

	int hmd_w, hmd_h;

	ohmd_context* ctx = ohmd_ctx_create();
	int num_devices = ohmd_ctx_probe(ctx);
	if(num_devices < 0) {
		printf("failed to probe devices: %s\n", ohmd_ctx_get_error(ctx));
		return 1;
	}

	/* For all devices that are not set by the user, choose first available */
	for(int i = 0; i < num_devices; i++) {
		int device_class = 0, device_flags = 0;
		ohmd_list_geti(ctx, i, OHMD_DEVICE_CLASS, &device_class);
		ohmd_list_geti(ctx, i, OHMD_DEVICE_FLAGS, &device_flags);
		if (hmddev == -1 && (device_class == OHMD_DEVICE_CLASS_HMD)) {
			hmddev = i;
		}
		if (lcontrollerdev == -1 && (device_class == OHMD_DEVICE_CLASS_CONTROLLER) && (device_flags & OHMD_DEVICE_FLAGS_LEFT_CONTROLLER)) {
			lcontrollerdev = i;
		}
		if (rcontrollerdev == -1 && (device_class == OHMD_DEVICE_CLASS_CONTROLLER) && (device_flags & OHMD_DEVICE_FLAGS_RIGHT_CONTROLLER)) {
			rcontrollerdev = i;
		}
	}

	ohmd_device_settings* settings = ohmd_device_settings_create(ctx);

	// If OHMD_IDS_AUTOMATIC_UPDATE is set to 0, ohmd_ctx_update() must be called at least 10 times per second.
	// It is enabled by default.

	int auto_update = 1;
	ohmd_device_settings_seti(settings, OHMD_IDS_AUTOMATIC_UPDATE, &auto_update);



	ohmd_device* hmd = ohmd_list_open_device_s(ctx, hmddev, settings);
	if(!hmd){
		printf("failed to open device: %s\n", ohmd_ctx_get_error(ctx));
		return 1;
	}

	ohmd_device* lc = ohmd_list_open_device_s(ctx, lcontrollerdev, settings);
	if(!lc){
		printf("failed to open device: %s\n", ohmd_ctx_get_error(ctx));
		return 1;
	}
	
	ohmd_device* rc = ohmd_list_open_device_s(ctx, rcontrollerdev, settings);
	if(!rc){
		printf("failed to open device: %s\n", ohmd_ctx_get_error(ctx));
		return 1;
	}
	
	printf("HMD:\n");
	printf("\t%s\n", ohmd_list_gets(ctx, hmddev, OHMD_PRODUCT));

	printf("Left controller:\n");
	printf("\t%s\n", ohmd_list_gets(ctx, lcontrollerdev, OHMD_PRODUCT));

	printf("Right controller:\n");
	printf("\t%s\n", ohmd_list_gets(ctx, rcontrollerdev, OHMD_PRODUCT));

	ohmd_device_geti(hmd, OHMD_SCREEN_HORIZONTAL_RESOLUTION, &hmd_w);
	ohmd_device_geti(hmd, OHMD_SCREEN_VERTICAL_RESOLUTION, &hmd_h);
	

	ohmd_device_settings_destroy(settings);

	gl_ctx gl;
	GLuint VAO[1];
	GLuint appshader;
	init_gl(&gl, hmd_w, hmd_h, VAO, &appshader);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(gl_debug_callback, 0);

	SimpleRenderer *renderer = openhmd_create_simple_renderer(hmd);
	if (!renderer)
		return 1;
	
	int eye_w = hmd_w/2*OVERSAMPLE_SCALE;
	int eye_h = hmd_h*OVERSAMPLE_SCALE;

	GLuint textures[2];
	GLuint framebuffers[2];
	GLuint depthbuffers[2];
	for (int i = 0; i < 2; i++)
		create_fbo(eye_w, eye_h, &framebuffers[i], &textures[i], &depthbuffers[i]);

	gen__cubes();

	SDL_ShowCursor(SDL_DISABLE);

	bool done = false;
	bool crosshair_overlay = false;
	while(!done){
		ohmd_ctx_update(ctx);

		SDL_Event event;
		while(SDL_PollEvent(&event)){
			if(event.type == SDL_KEYDOWN){
				switch(event.key.keysym.sym){
				case SDLK_ESCAPE:
					done = true;
					break;
				case SDLK_F1:
					{
						gl.is_fullscreen = !gl.is_fullscreen;
						SDL_SetWindowFullscreen(gl.window, gl.is_fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
					}
					break;
				case SDLK_F2:
					{
						// reset rotation and position
						float zero[] = {0, 0, 0, 1};
						ohmd_device_setf(hmd, OHMD_ROTATION_QUAT, zero);
						ohmd_device_setf(hmd, OHMD_POSITION_VECTOR, zero);
					}
					break;
				case SDLK_F3:
					{
						float mat[16];
						ohmd_device_getf(hmd, OHMD_LEFT_EYE_GL_PROJECTION_MATRIX, mat);
						printf("Projection L: ");
						print_matrix(mat);
						printf("\n");
						ohmd_device_getf(hmd, OHMD_RIGHT_EYE_GL_PROJECTION_MATRIX, mat);
						printf("Projection R: ");
						print_matrix(mat);
						printf("\n");
						ohmd_device_getf(hmd, OHMD_LEFT_EYE_GL_MODELVIEW_MATRIX, mat);
						printf("View: ");
						print_matrix(mat);
						printf("\n");
						printf("viewport_scale: [%0.4f, %0.4f]\n", renderer->viewport_scale[0], renderer->viewport_scale[1]);
						printf("lens separation: %04f\n", renderer->sep);
						printf("IPD: %0.4f\n", renderer->ipd);
						printf("warp_scale: %0.4f\r\n", renderer->warp_scale);
						printf("distortion coeffs: [%0.4f, %0.4f, %0.4f, %0.4f]\n", renderer->distortion_coeffs[0], renderer->distortion_coeffs[1], renderer->distortion_coeffs[2], renderer->distortion_coeffs[3]);
						printf("aberration coeffs: [%0.4f, %0.4f, %0.4f]\n", renderer->aberr_scale[0], renderer->aberr_scale[1], renderer->aberr_scale[2]);
						printf("left_lens_center: [%0.4f, %0.4f]\n", renderer->left_lens_center[0], renderer->left_lens_center[1]);
						printf("right_lens_center: [%0.4f, %0.4f]\n", renderer->right_lens_center[0], renderer->right_lens_center[1]);
					}
					break;
				case SDLK_w:
					renderer->sep += 0.001;
					renderer->left_lens_center[0] = renderer->viewport_scale[0] - renderer->sep/2.0f;
					renderer->right_lens_center[0] = renderer->sep/2.0f;
					break;
				case SDLK_q:
					renderer->sep -= 0.001;
					renderer->left_lens_center[0] = renderer->viewport_scale[0] - renderer->sep/2.0f;
					renderer->right_lens_center[0] = renderer->sep/2.0f;
					break;
				case SDLK_a:
					renderer->warp_adj *= 1.0/0.9;
					break;
				case SDLK_z:
					renderer->warp_adj *= 0.9;
					break;
				case SDLK_i:
					renderer->ipd -= 0.001;
					ohmd_device_setf(hmd, OHMD_EYE_IPD, &renderer->ipd);
					break;
				case SDLK_o:
					renderer->ipd += 0.001;
					ohmd_device_setf(hmd, OHMD_EYE_IPD, &renderer->ipd);
					break;
				case SDLK_d:
					/* toggle between distorted and undistorted views */
					if ((renderer->distortion_coeffs[0] != 0.0) ||
							(renderer->distortion_coeffs[1] != 0.0) ||
							(renderer->distortion_coeffs[2] != 0.0) ||
							(renderer->distortion_coeffs[3] != 1.0)) {
						renderer->distortion_coeffs[0] = 0.0;
						renderer->distortion_coeffs[1] = 0.0;
						renderer->distortion_coeffs[2] = 0.0;
						renderer->distortion_coeffs[3] = 1.0;
					} else {
						ohmd_device_getf(hmd, OHMD_UNIVERSAL_DISTORTION_K, &(renderer->distortion_coeffs[0]));
					}
					break;
				case SDLK_x:
					crosshair_overlay = ! crosshair_overlay;
					break;
				default:
					break;
				}
			}
		}

		glViewport(0, 0, eye_w, eye_h);
		glScissor(0, 0, eye_w, eye_h);

		glUseProgram(appshader);
		for (int i = 0; i < 2; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthbuffers[i], 0);

			glClearColor(0.0, 0, 0.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(VAO[0]);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_SCISSOR_TEST);

			float projectionmatrix[16];
			ohmd_device_getf(hmd, OHMD_LEFT_EYE_GL_PROJECTION_MATRIX, projectionmatrix);
			glUniformMatrix4fv(glGetUniformLocation(appshader, "proj"), 1, GL_FALSE, projectionmatrix);

			float viewmatrix[16];
			ohmd_device_getf(hmd, OHMD_LEFT_EYE_GL_MODELVIEW_MATRIX, viewmatrix);
			glUniformMatrix4fv(glGetUniformLocation(appshader, "view"), 1, GL_FALSE, viewmatrix);

			draw_cubes(appshader);
			draw_controllers(appshader, lc, rc);
		}

		openhmd_render_textures(renderer, textures[0], textures[1]);
	}

	ohmd_ctx_destroy(ctx);

	return 0;
}
