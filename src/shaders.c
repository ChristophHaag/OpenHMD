#include "shaders.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

coordinate ComputeDistortion(float fU, float fV, float viewport_scale[], float lens_center[], float warp_scale, float distortion_coeffs[], float aberr_scale[]) {

	float r[2];
	r[0] = fU * viewport_scale[0] - lens_center[0];
	r[1] = fV * viewport_scale[1] - lens_center[1];

	//printf("%f, %f, %f, %f, %f %f, %f\n", fU, fV, lens_center[0], lens_center[1], r[0], r[1], warp_scale);

	r[0] /= warp_scale;
	r[1] /= warp_scale;

	float r_mag = sqrt(r[0] * r[0] + r[1] * r[1]);
	//printf("center %f %f, in %f, %f, dists %f %f, dist %f\n", lens_center[0], lens_center[1], fU, fV, r[0], r[1], r_mag);


	float r_displaced[2];
	r_displaced[0] = r[0] * (distortion_coeffs[3] + distortion_coeffs[2] * r_mag + distortion_coeffs[1] * r_mag * r_mag + distortion_coeffs[0] * r_mag * r_mag * r_mag);

	r_displaced[1] = r[1] * (distortion_coeffs[3] + distortion_coeffs[2] * r_mag + distortion_coeffs[1] * r_mag * r_mag + distortion_coeffs[0] * r_mag * r_mag * r_mag);

	r_displaced[0] *= warp_scale;
	r_displaced[1] *= warp_scale;

	float tc_r[2];
	tc_r[0] = (lens_center[0] + aberr_scale[0] * r_displaced[0]) / viewport_scale[0];
	tc_r[1] = (lens_center[1] + aberr_scale[0] * r_displaced[1]) / viewport_scale[1];

	float tc_g[2];
	tc_g[0] = (lens_center[0] + aberr_scale[1] * r_displaced[0]) / viewport_scale[0];
	tc_g[1] = (lens_center[1] + aberr_scale[1] * r_displaced[1]) / viewport_scale[1];

	float tc_b[2];
	tc_b[0] = (lens_center[0] + aberr_scale[2] * r_displaced[0]) / viewport_scale[0];
	tc_b[1] = (lens_center[1] + aberr_scale[2] * r_displaced[1]) / viewport_scale[1];

	//DriverLog("Distort %f %f -> %f %f; %f %f %f %f\n", fU, fV, tc_b[0], tc_b[1], distortion_coeffs[0], distortion_coeffs[1], distortion_coeffs[2], distortion_coeffs[3]);
	//printf("%f, %f\n", tc_r[0], tc_r[1]);

	coordinate c;
	c.tc_r_x = tc_r[0];
	c.tc_r_y = tc_r[1];
	c.tc_g_x = tc_g[0];
	c.tc_g_y = tc_g[1];
	c.tc_b_x = tc_b[0];
	c.tc_b_y = tc_b[1];
	return c;
}

distortionCoordinates calculateDistortion(ohmd_device* hmd) {
	int hmd_w;
	int hmd_h;
	ohmd_device_geti(hmd, OHMD_SCREEN_HORIZONTAL_RESOLUTION, &hmd_w);
	ohmd_device_geti(hmd, OHMD_SCREEN_VERTICAL_RESOLUTION, &hmd_h);

	distortionCoordinates result;
	printf("Allocating %lu\n", hmd_w / 2 * hmd_h * sizeof(coordinate));

	result.distortionLeft = malloc(hmd_w / 2 * hmd_h * sizeof(coordinate));
	printf("distortionLeft %p\n", result.distortionLeft);
	result.distortionRight = malloc(hmd_w / 2 * hmd_h * sizeof(coordinate));
	result.height = hmd_h;
	result.width = hmd_w / 2;

	float viewport_scale[2];
	float distortion_coeffs[4];
	float aberr_scale[3];
	float sep;
	float left_lens_center[2];
	float right_lens_center[2];
	//viewport is half the screen
	ohmd_device_getf(hmd, OHMD_SCREEN_HORIZONTAL_SIZE, &(viewport_scale[0]));
	viewport_scale[0] /= 2.0f;
	ohmd_device_getf(hmd, OHMD_SCREEN_VERTICAL_SIZE, &(viewport_scale[1]));
	//distortion coefficients
	ohmd_device_getf(hmd, OHMD_UNIVERSAL_DISTORTION_K, &(distortion_coeffs[0]));
	ohmd_device_getf(hmd, OHMD_UNIVERSAL_ABERRATION_K, &(aberr_scale[0]));
	//calculate lens centers (assuming the eye separation is the distance betweenteh lense centers)
	ohmd_device_getf(hmd, OHMD_LENS_HORIZONTAL_SEPARATION, &sep);
	ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(left_lens_center[1]));
	ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(right_lens_center[1]));
	left_lens_center[0] = viewport_scale[0] - sep/2.0f;
	right_lens_center[0] = sep/2.0f;
	//asume calibration was for lens view to which ever edge of screen is further away from lens center
	float warp_scale = (left_lens_center[0] > right_lens_center[0]) ? left_lens_center[0] : right_lens_center[0];

	float lens_center[2];
	//lens_center[0] = (eEye == 0 ? left_lens_center[0] : right_lens_center[0]);
	//lens_center[1] = (eEye == 0 ? left_lens_center[1] : right_lens_center[1]);

	printf("Precalculating distortion for, viewport scale %f,%f, warp scale %f and coefficients %f %f %f\n", viewport_scale[0], viewport_scale[1], warp_scale, distortion_coeffs[0], distortion_coeffs[1], distortion_coeffs[2]);

	for (int row = 0; row < result.width; row++) {
		for (int col = 0; col < result.width; col++) {
			coordinate c = ComputeDistortion(row / (float)result.height, col / (float) result.width, viewport_scale, left_lens_center, warp_scale, distortion_coeffs, aberr_scale);
			result.distortionLeft[row * col + col] = c;
			//printf("%f, %f\n", c.tc_r_x, c.tc_r_y);
		}
	}

	for (int row = 0; row < result.width; row++) {
		for (int col = 0; col < result.width; col++) {
			//coordinate c = ComputeDistortion(row, col, viewport_scale, right_lens_center, warp_scale, distortion_coeffs, aberr_scale);
			//result.distortionRight[row * col + col] = c;
		}
	}
	return result;
}

const char * const distortion_vert =
"#version 120\n"
"void main(void)\n"
"{\n"
    "gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n"
"}";

const char * const distortion_frag =
"#version 120\n"
"\n"
"//per eye texture to warp for lens distortion\n"
"uniform sampler2D warpTexture;\n"
"\n"
"//Position of lens center in m (usually eye_w/2, eye_h/2)\n"
"uniform vec2 LensCenter;\n"
"//Scale from texture co-ords to m (usually eye_w, eye_h)\n"
"uniform vec2 ViewportScale;\n"
"//Distortion overall scale in m (usually ~eye_w/2)\n"
"uniform float WarpScale;\n"
"//Distoriton coefficients (PanoTools model) [a,b,c,d]\n"
"uniform vec4 HmdWarpParam;\n"
"\n"
"//chromatic distortion post scaling\n"
"uniform vec3 aberr;\n"
"\n"
"void main()\n"
"{\n"
    "//output_loc is the fragment location on screen from [0,1]x[0,1]\n"
    "vec2 output_loc = vec2(gl_TexCoord[0].s, gl_TexCoord[0].t);\n"
    "//Compute fragment location in lens-centered co-ordinates at world scale\n"
	"vec2 r = output_loc * ViewportScale - LensCenter;\n"
    "//scale for distortion model\n"
    "//distortion model has r=1 being the largest circle inscribed (e.g. eye_w/2)\n"
    "r /= WarpScale;\n"
"\n"
    "//|r|**2\n"
    "float r_mag = length(r);\n"
    "//offset for which fragment is sourced\n"
    "vec2 r_displaced = r * (HmdWarpParam.w + HmdWarpParam.z * r_mag +\n"
    "HmdWarpParam.y * r_mag * r_mag +\n"
    "HmdWarpParam.x * r_mag * r_mag * r_mag);\n"
    "//back to world scale\n"
    "r_displaced *= WarpScale;\n"
    "//back to viewport co-ord\n"
    "vec2 tc_r = (LensCenter + aberr.r * r_displaced) / ViewportScale;\n"
    "vec2 tc_g = (LensCenter + aberr.g * r_displaced) / ViewportScale;\n"
    "vec2 tc_b = (LensCenter + aberr.b * r_displaced) / ViewportScale;\n"
"\n"
    "float red = texture2D(warpTexture, tc_r).r;\n"
    "float green = texture2D(warpTexture, tc_g).g;\n"
    "float blue = texture2D(warpTexture, tc_b).b;\n"
    "//Black edges off the texture\n"
    "gl_FragColor = ((tc_g.x < 0.0) || (tc_g.x > 1.0) || (tc_g.y < 0.0) || (tc_g.y > 1.0)) ? vec4(0.0, 0.0, 0.0, 1.0) : vec4(red, green, blue, 1.0);\n"
"}";

const char * const distortion_vert2 =
"#version 120\n"
"attribute vec2 atc_r;\n"
"attribute vec2 atc_g;\n"
"attribute vec2 atc_b;\n"
"varying vec2 tc_r;\n"
"varying vec2 tc_g;\n"
"varying vec2 tc_b;\n"
"void main(void)\n"
"{\n"
    "gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n"
    "tc_r = atc_r;\n"
    "tc_g = atc_g;\n"
    "tc_b = atc_b;\n"
"}";

const char * const distortion_frag2 =
"#version 120\n"
"\n"
"//per eye texture to warp for lens distortion\n"
"uniform sampler2D warpTexture;\n"
"\n"

"//Distortion coordinates\n"
"varying vec2 tc_r;\n"
"varying vec2 tc_g;\n"
"varying vec2 tc_b;\n"
"\n"
"void main()\n"
"{\n"
    "//output_loc is the fragment location on screen from [0,1]x[0,1]\n"
    "vec2 output_loc = vec2(gl_TexCoord[0].s, gl_TexCoord[0].t);\n"

    "//vec2 tc_r = output_loc;\n"
    "//vec2 tc_g = output_loc;\n"
    "//vec2 tc_b = output_loc;\n"
"\n"
    "float red = texture2D(warpTexture, tc_r).r;\n"
    "float green = texture2D(warpTexture, tc_g).g;\n"
    "float blue = texture2D(warpTexture, tc_b).b;\n"
    "//Black edges off the texture\n"
    "gl_FragColor = ((tc_g.x < 0.0) || (tc_g.x > 1.0) || (tc_g.y < 0.0) || (tc_g.y > 1.0)) ? vec4(0.0, 0.0, 0.0, 1.0) : vec4(red, green, blue, 1.0);\n"
"}";

const char *const distortion_vert_330 =
"#version 330\n"
"\n"
"layout (location=0) in vec2 coords;"
"uniform mat4 mvp;"
"out vec2 T;"
"\n"
"void main(void)\n"
"{\n"
    "T = coords;\n"
    "gl_Position = mvp * vec4(coords, 0.0, 1.0);\n"
"}";

const char *const distortion_frag_330 =
"#version 330\n"
"\n"
"//per eye texture to warp for lens distortion\n"
"uniform sampler2D warpTexture;\n"
"\n"
"//Position of lens center in m (usually eye_w/2, eye_h/2)\n"
"uniform vec2 LensCenter;\n"
"//Scale from texture co-ords to m (usually eye_w, eye_h)\n"
"uniform vec2 ViewportScale;\n"
"//Distortion overall scale in m (usually ~eye_w/2)\n"
"uniform float WarpScale;\n"
"//Distoriton coefficients (PanoTools model) [a,b,c,d]\n"
"uniform vec4 HmdWarpParam;\n"
"\n"
"//chromatic distortion post scaling\n"
"uniform vec3 aberr;\n"
"\n"
"in vec2 T;\n"
"out vec4 color;\n"
"\n"
"void main()\n"
"{\n"
    "//output_loc is the fragment location on screen from [0,1]x[0,1]\n"
    "vec2 output_loc = vec2(T.s, T.t);\n"
    "//Compute fragment location in lens-centered co-ordinates at world scale\n"
    "vec2 r = output_loc * ViewportScale - LensCenter;\n"
    "//scale for distortion model\n"
    "//distortion model has r=1 being the largest circle inscribed (e.g. eye_w/2)\n"
    "r /= WarpScale;\n"
    "\n"
    "//|r|**2\n"
    "float r_mag = length(r);\n"
    "//offset for which fragment is sourced\n"
    "vec2 r_displaced = r * (HmdWarpParam.w + HmdWarpParam.z * r_mag +\n"
    "HmdWarpParam.y * r_mag * r_mag +\n"
    "HmdWarpParam.x * r_mag * r_mag * r_mag);\n"
    "//back to world scale\n"
    "r_displaced *= WarpScale;\n"
    "//back to viewport co-ord\n"
    "vec2 tc_r = (LensCenter + aberr.r * r_displaced) / ViewportScale;\n"
    "vec2 tc_g = (LensCenter + aberr.g * r_displaced) / ViewportScale;\n"
    "vec2 tc_b = (LensCenter + aberr.b * r_displaced) / ViewportScale;\n"
"\n"
    "float red = texture(warpTexture, tc_r).r;\n"
    "float green = texture(warpTexture, tc_g).g;\n"
    "float blue = texture(warpTexture, tc_b).b;\n"
    "//Black edges off the texture\n"
    "color = ((tc_g.x < 0.0) || (tc_g.x > 1.0) || (tc_g.y < 0.0) || (tc_g.y > 1.0)) ? vec4(0.0, 0.0, 0.0, 1.0) : vec4(red, green, blue, 1.0);\n"
"}";
