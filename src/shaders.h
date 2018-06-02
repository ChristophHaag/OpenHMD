#ifndef SHADERS_H
#define SHADERS_H

#include <openhmd.h>

/** Used to return the post-distortion UVs for each color channel.
 * UVs range from 0 to 1 with 0,0 in the upper left corner of the
 * source render target. The 0,0 to 1,1 range covers a single eye. */
typedef struct {
	float tc_r_x;
	float tc_r_y;
	float tc_g_x;
	float tc_g_y;
	float tc_b_x;
	float tc_b_y;
} coordinate;

typedef struct {
	coordinate* distortionLeft;
	coordinate* distortionRight;
	int width;
	int height;
} distortionCoordinates;

distortionCoordinates calculateDistortion(ohmd_device* device);

extern const char * const distortion_vert;
extern const char * const distortion_frag;

extern const char * const distortion_vert2;
extern const char * const distortion_frag2;

extern const char * const distortion_vert_330;
extern const char * const distortion_frag_330;

#endif /* SHADERS_H */
