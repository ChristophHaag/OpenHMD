/*
 * OpenHMD - Free and Open Source API and drivers for immersive technology.
 * Copyright (C) 2013 Fredrik Hultin.
 * Copyright (C) 2013 Jakob Bornecrantz.
 * Distributed under the Boost 1.0 licence, see LICENSE for full text.
 */

/* HTC Vive Survive Driver */
#define HTC_ID                   0x0bb4
#define VIVE_HMD                 0x2c87
#define VIVE_PRO_HMD             0x0309

#define VALVE_ID                 0x28de
#define VIVE_WATCHMAN_DONGLE     0x2101
#define VIVE_LIGHTHOUSE_FPGA_RX  0x2000
#define VIVE_LHR                 0x2300 // VIVE PRO

#define VIVE_CLOCK_FREQ 48000000.0f // Hz = 48 MHz

#define VIVE_PRO_MAINBOARD_PID 0x0309
#define VIVE_WATCHMAN_DONGLE_GEN2 0x2102

#include <string.h>
#include <wchar.h>
#include <hidapi.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

#include <libsurvive/survive_api.h>
#include "../openhmdi.h"

typedef struct vive_priv_struct vive_priv;
typedef struct {
	SurviveSimpleContext *actx;
	vive_priv* hmd;
	vive_priv* lc;
	vive_priv* rc;
} vive_shared;

typedef struct vive_priv_struct {
	ohmd_device base;
	vive_shared* shared;
	int id;

	double libsurvive_pos[3];
	double libsurvive_quat[4];
} vive_priv;

quatf abs_rotate_offset = { -sqrt(0.5), 0, 0 ,sqrt(0.5) };

static int getf(ohmd_device* device, ohmd_float_value type, float* out)
{
	vive_priv* priv = (vive_priv*)device;

	switch(type){
	case OHMD_ROTATION_QUAT:
  {
    quatf rotation;
      rotation.x = (FLT) priv->libsurvive_quat[0];
      rotation.y = (FLT) priv->libsurvive_quat[1];
      rotation.z = (FLT) priv->libsurvive_quat[2];
      rotation.w = (FLT) priv->libsurvive_quat[3];

      //rotation 90° around X axis
      oquatf_mult(&abs_rotate_offset, &rotation, (quatf*) out);
    }
		break;

	case OHMD_POSITION_VECTOR:
		out[0] = (FLT) priv->libsurvive_pos[0];
		out[1] = (FLT) priv->libsurvive_pos[1];
		out[2] = (FLT) priv->libsurvive_pos[2];

    oquatf_get_rotated(&abs_rotate_offset, (vec3f*) out, (vec3f*) out);
		break;

	case OHMD_DISTORTION_K:
		// TODO this should be set to the equivalent of no distortion
		memset(out, 0, sizeof(float) * 6);
		break;

	default:
		ohmd_set_error(priv->base.ctx, "invalid type given to getf (%ud)", type);
		return -1;
		break;
	}

	return 0;
}

static void close_device(ohmd_device* device)
{
	vive_priv* priv = (vive_priv*) device;
	survive_simple_close(priv->shared->actx);
}

static vive_priv* drv_priv_get(ohmd_device* device)
{
	return (vive_priv*)device;
}

static void update_device(ohmd_device* device)
{
	vive_priv* priv = drv_priv_get(device);

	SurvivePose pose;
	//TODO: this updates hmd and all controllers whenever update is called for any of them
	for (const SurviveSimpleObject *it = survive_simple_get_first_object(priv->shared->actx); it != 0; it = survive_simple_get_next_object(priv->shared->actx, it)) {
		double* openhmd_position;
		double* openhmd_rotation;

		uint32_t timecode = survive_simple_object_get_latest_pose(it, &pose);

		const char* codename = survive_simple_object_name(it);
		//printf("%s (%u): %f %f %f %f %f %f %f\n", survive_simple_object_name(it), timecode, pose.Pos[0], pose.Pos[1], pose.Pos[2], pose.Rot[0], pose.Rot[1], pose.Rot[2], pose.Rot[3]);
    if (strcmp(codename, "HMD") == 0 || strcmp(codename, "T20") == 0) {
			//printf("Pose: [%u][%s][% 08.8f,% 08.8f,% 08.8f] [% 08.8f,% 08.8f,% 08.8f,% 08.8f]\n", timecode, codename, pose.Pos[0], pose.Pos[1], pose.Pos[2], pose.Rot[0], pose.Rot[1], pose.Rot[2], pose.Rot[3]);
			openhmd_position = priv->shared->hmd->libsurvive_pos;
			openhmd_rotation = priv->shared->hmd->libsurvive_quat;
    } else if (strcmp(codename, "WM0") == 0 || strcmp(codename, "KN0") == 0) {
			if (priv->shared->lc == NULL) continue; // app doesn't use lc
			//printf("Pose: [%u][%s][% 08.8f,% 08.8f,% 08.8f] [% 08.8f,% 08.8f,% 08.8f,% 08.8f]\n", timecode, codename, pose.Pos[0], pose.Pos[1], pose.Pos[2], pose.Rot[0], pose.Rot[1], pose.Rot[2], pose.Rot[3]);
			//printf("Controller 0 Pose: [%1.1x][%s][% 08.8f,% 08.8f,% 08.8f] [% 08.8f,% 08.8f,% 08.8f,% 08.8f]\n", lighthouse, so->codename, pose->Pos[0], pose->Pos[1], pose->Pos[2], pose->Rot[0], pose->Rot[1], pose->Rot[2], pose->Rot[3]);
			openhmd_position = priv->shared->lc->libsurvive_pos;
			openhmd_rotation = priv->shared->lc->libsurvive_quat;
    } else if (strcmp(codename, "WM1") == 0 || strcmp(codename, "KN1") == 0) {
			if (priv->shared->rc == NULL) continue; // app doesn't use rc
			//printf("Pose: [%u][%s][% 08.8f,% 08.8f,% 08.8f] [% 08.8f,% 08.8f,% 08.8f,% 08.8f]\n", timecode, codename, pose.Pos[0], pose.Pos[1], pose.Pos[2], pose.Rot[0], pose.Rot[1], pose.Rot[2], pose.Rot[3]);
			//printf("Controller 1 Pose: [%1.1x][%s][% 08.8f,% 08.8f,% 08.8f] [% 08.8f,% 08.8f,% 08.8f,% 08.8f]\n", lighthouse, so->codename, pose->Pos[0], pose->Pos[1], pose->Pos[2], pose->Rot[0], pose->Rot[1], pose->Rot[2], pose->Rot[3]);
			openhmd_position = priv->shared->rc->libsurvive_pos;
			openhmd_rotation = priv->shared->rc->libsurvive_quat;
		} else {
			// LH0 etc
			continue;
		}

		openhmd_position[0] =  pose.Pos[0];
		openhmd_position[1] =  pose.Pos[1];
		openhmd_position[2] =  pose.Pos[2];

		openhmd_rotation[0] /* x */ =  pose.Rot[1];
		openhmd_rotation[1] /* y */ =  pose.Rot[2];
		openhmd_rotation[2] /* z */ =  pose.Rot[3];
		openhmd_rotation[3] /* w */ =  pose.Rot[0];
	}

}
static vive_shared* shared; //TODO: get rid of this
static ohmd_device* open_device(ohmd_driver* driver, ohmd_device_desc* desc)
{
	vive_priv* priv = ohmd_alloc(driver->ctx, sizeof(vive_priv));

	if(!priv) {
		printf("alloc failed!\n");
		return NULL;
	}

	if (!shared) {
		printf("Initializing shared libsurvive access...\n");
		shared = malloc(sizeof(vive_shared));
		shared->lc = NULL;
		shared->rc = NULL;
		shared->actx = survive_simple_init(0, 0);
		if (shared->actx == 0) { // implies -help or similiar
			printf("Error initializing libsurvive\n");
			return NULL;
		}

		survive_simple_start_thread(shared->actx);
		//printf("thread %d creates libsurvive thread\n", pthread_self());
	}
	priv->shared = shared;

	priv->id = desc->id;

	priv->base.ctx = driver->ctx;

	if (priv->id == 0) {
		priv->shared->hmd = priv;
	} else if (priv->id == 1) {
		priv->shared->lc = priv;
		return (ohmd_device*)priv;
	} else if (priv->id == 2) {
		priv->shared->rc = priv;
		return (ohmd_device*)priv;
	}

	// Set default device properties
	ohmd_set_default_device_properties(&priv->base.properties);

	// Set device properties TODO: Get from device
	priv->base.properties.hsize = 0.122822f;
	priv->base.properties.vsize = 0.068234f;
	priv->base.properties.hres = 2160;
	priv->base.properties.vres = 1200;
	/*
	 *    // calculated from here: https://www.gamedev.net/topic/683698-projection-matrix-model-of-the-htc-vive/
	 *	priv->base.properties.lens_sep = 0.057863;
	 *	priv->base.properties.lens_vpos = 0.033896;
	 */
	// estimated 'by eye' on jsarret's vive
	priv->base.properties.lens_sep = 0.056;
	priv->base.properties.lens_vpos = 0.032;
	float eye_to_screen_distance = 0.023226876441867737;
	//priv->base.properties.fov = DEG_TO_RAD(111.435f); //TODO: Confirm exact mesurements
	priv->base.properties.ratio = (2160.0f / 1200.0f) / 2.0f;

	/*
	 *    ohmd_set_universal_distortion_k(&(priv->base.properties), 0.394119, -0.508383, 0.323322, 0.790942);
	 */
	ohmd_set_universal_distortion_k(&(priv->base.properties), 1.318397, -1.490242, 0.663824, 0.508021);
	ohmd_set_universal_aberration_k(&(priv->base.properties), 1.00010147892f, 1.000f, 1.00019614479f);


	// calculate projection eye projection matrices from the device properties
	//ohmd_calc_default_proj_matrices(&priv->base.properties);
	float l,r,t,b,n,f;
	// left eye screen bounds
	l = -1.0f * (priv->base.properties.hsize/2 - priv->base.properties.lens_sep/2);
	r = priv->base.properties.lens_sep/2;
	t = priv->base.properties.vsize - priv->base.properties.lens_vpos;
	b = -1.0f * priv->base.properties.lens_vpos;
	n = eye_to_screen_distance;
	f = n*10e6;
	//LOGD("l: %0.3f, r: %0.3f, b: %0.3f, t: %0.3f, n: %0.3f, f: %0.3f", l,r,b,t,n,f);
	/* eye separation is handled by IPD in the Modelview matrix */
	omat4x4f_init_frustum(&priv->base.properties.proj_left, l, r, b, t, n, f);
	//right eye screen bounds
	l = -1.0f * priv->base.properties.lens_sep/2;
	r = priv->base.properties.hsize/2 - priv->base.properties.lens_sep/2;
	n = eye_to_screen_distance;
	f = n*10e6;
	//LOGD("l: %0.3f, r: %0.3f, b: %0.3f, t: %0.3f, n: %0.3f, f: %0.3f", l,r,b,t,n,f);
	/* eye separation is handled by IPD in the Modelview matrix */
	omat4x4f_init_frustum(&priv->base.properties.proj_right, l, r, b, t, n, f);

	priv->base.properties.fov = 2 * atan2f(
		priv->base.properties.hsize/2 - priv->base.properties.lens_sep/2,
		eye_to_screen_distance);

	// set up device callbacks
	priv->base.update = update_device;
	priv->base.getf = getf;
	priv->base.close = close_device;

	return (ohmd_device*)priv;
}

static void get_device_list(ohmd_driver* driver, ohmd_device_list* list)
{
	struct hid_device_info* devs = hid_enumerate(HTC_ID, VIVE_HMD);
  if (devs == NULL) {
    devs = hid_enumerate(VALVE_ID, VIVE_LHR);
  }

	struct hid_device_info* cur_dev = devs;

	int idx = 0;
	while (cur_dev) {
		ohmd_device_desc* desc = &list->devices[list->num_devices++];

		strcpy(desc->driver, "OpenHMD HTC Vive Driver (libsurvive)");
		strcpy(desc->vendor, "HTC/Valve");
		strcpy(desc->product, "HTC Vive");

		desc->revision = 0;

		snprintf(desc->path, OHMD_STR_SIZE, "%d", idx);

		desc->driver_ptr = driver;
		desc->device_flags = OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING | OHMD_DEVICE_FLAGS_ROTATIONAL_TRACKING;
		desc->device_class = OHMD_DEVICE_CLASS_HMD;

		desc->id = idx++;

		//Controller 0
		desc = &list->devices[list->num_devices++];

		strcpy(desc->driver, "OpenHMD HTC Vive Driver (libsurvive)");
		strcpy(desc->vendor, "HTC/Valve");
		strcpy(desc->product, "HTC Vive: Controller 0");

		strcpy(desc->path, cur_dev->path);

		desc->device_flags =
		OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING |
		OHMD_DEVICE_FLAGS_ROTATIONAL_TRACKING |
		OHMD_DEVICE_FLAGS_RIGHT_CONTROLLER;

		desc->driver_ptr = driver;
		desc->id = idx++;

		// Controller 1
		desc = &list->devices[list->num_devices++];

		strcpy(desc->driver, "OpenHMD HTC Vive Driver (libsurvive)");
		strcpy(desc->vendor, "HTC/Valve");
		strcpy(desc->product, "HTC Vive: Controller 1");

		strcpy(desc->path, cur_dev->path);

		desc->device_flags =
		OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING |
		OHMD_DEVICE_FLAGS_ROTATIONAL_TRACKING |
		OHMD_DEVICE_FLAGS_LEFT_CONTROLLER;

		desc->driver_ptr = driver;
		desc->id = idx++;

		cur_dev = cur_dev->next;
	}

	hid_free_enumeration(devs);
}

static void destroy_driver(ohmd_driver* drv)
{
	LOGD("shutting down HTC Vive driver");
	free(drv);
}

ohmd_driver* ohmd_create_htc_vive_survive_drv(ohmd_context* ctx)
{
	ohmd_driver* drv = ohmd_alloc(ctx, sizeof(ohmd_driver));

	if(!drv)
		return NULL;

	drv->get_device_list = get_device_list;
	drv->open_device = open_device;
	drv->destroy = destroy_driver;
	drv->ctx = ctx;

	return drv;
}
