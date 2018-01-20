/*
 * OpenHMD - Free and Open Source API and drivers for immersive technology.
 * Copyright (C) 2013 Fredrik Hultin.
 * Copyright (C) 2013 Jakob Bornecrantz.
 * Distributed under the Boost 1.0 licence, see LICENSE for full text.
 */

/* HTC Vive Driver */

#define HTC_ID                   0x0bb4
#define VIVE_HMD                 0x2c87

#include <string.h>
#include <wchar.h>
#include <hidapi.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

#define USE_DOUBLE
#include <survive.h>
#undef USE_DOUBLE

#include "vive.h"

typedef struct {
	ohmd_device base;
	double libsurvive_pos[3];
	double libsurvive_quat[4];

	struct SurviveContext * libsurvive_ctx;

	ohmd_thread* survive_poll_thread;
	ohmd_mutex* survive_copy_mutex;
} vive_priv;

static int getf(ohmd_device* device, ohmd_float_value type, float* out)
{
	vive_priv* priv = (vive_priv*)device;

	ohmd_lock_mutex(priv->survive_copy_mutex);
	switch(type){
	case OHMD_ROTATION_QUAT:
		// libsurvive_quat is 0=w 1=x 2=y 3=z
		// out is 0=x 1=y 2=z 3=w
		out[0] = (FLT) priv->libsurvive_quat[1];
		out[1] = (FLT) priv->libsurvive_quat[2];
		out[2] = (FLT) priv->libsurvive_quat[3];
		out[3] = (FLT) priv->libsurvive_quat[0];
		break;

	case OHMD_POSITION_VECTOR:
		out[0] = (FLT) priv->libsurvive_pos[0];
		out[1] = (FLT) priv->libsurvive_pos[1];
		out[2] = (FLT) priv->libsurvive_pos[2];
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

	ohmd_unlock_mutex(priv->survive_copy_mutex);
	return 0;
}

static void close_device(ohmd_device* device)
{
	vive_priv* priv = (vive_priv*) device;
	 ohmd_destroy_thread(priv->survive_poll_thread);
	 ohmd_destroy_mutex(priv->survive_copy_mutex);
}


void libsurvive_button_callback(SurviveObject * so, uint8_t eventType, uint8_t buttonId, uint8_t axis1Id, uint16_t axis1Val, uint8_t axis2Id, uint16_t axis2Val)
{
	survive_default_button_process(so, eventType, buttonId, axis1Id, axis1Val, axis2Id, axis2Val);

	// do nothing.
	printf("ButtonEntry: eventType:%x, buttonId:%d, axis1:%d, axis1Val:%8.8x, axis2:%d, axis2Val:%8.8x\n",
	       eventType,
	buttonId,
	axis1Id,
	axis1Val,
	axis2Id,
	axis2Val);

	// Note: the code for haptic feedback is not yet written to support wired USB connections to the controller.
	// Work is still needed to reverse engineer that USB protocol.

}


void libsurvive_raw_pose_callback(SurviveObject * so, uint8_t lighthouse, double *pos, double *quat)
{
	survive_default_raw_pose_process(so, lighthouse, pos, quat);

	// use pose of only lighthouse 0
	if (strcmp(so->codename, "HMD") == 0 && lighthouse == 0) {

		printf("Pose: [%1.1x][%s][% 08.8f,% 08.8f,% 08.8f] [% 08.8f,% 08.8f,% 08.8f,% 08.8f]\n", lighthouse, so->codename, pos[0], pos[1], pos[2], quat[0], quat[1], quat[2], quat[3]);

		vive_priv* priv = so->ctx->user_ptr;
		ohmd_lock_mutex(priv->survive_copy_mutex);
		priv->libsurvive_pos[0] = pos[0];
		priv->libsurvive_pos[1] = pos[1];
		priv->libsurvive_pos[2] = pos[2];

		priv->libsurvive_quat[0] = quat[0];
		priv->libsurvive_quat[1] = quat[1];
		priv->libsurvive_quat[2] = quat[2];
		priv->libsurvive_quat[3] = quat[3];
		ohmd_unlock_mutex(priv->survive_copy_mutex);
	}
}

void libsurvive_imu_callback(SurviveObject * so, int mask, double * accelgyromag, uint32_t timecode, int id)
{
	survive_default_imu_process(so, mask, accelgyromag, timecode, id);
}


static unsigned int survive_poll_thread(void* argument) {
	vive_priv* priv = (vive_priv*) argument;
	priv->libsurvive_ctx = survive_init( 0 );
	priv->libsurvive_ctx->user_ptr = argument; // to pass our vive_priv struct to the testprog_raw_pose_process callback

	if( !priv->libsurvive_ctx )
	{
		fprintf( stderr, "Fatal. Could not start\n" );
	}

	survive_install_button_fn(priv->libsurvive_ctx, libsurvive_button_callback);
	survive_install_raw_pose_fn(priv->libsurvive_ctx, libsurvive_raw_pose_callback);
	survive_install_imu_fn(priv->libsurvive_ctx, libsurvive_imu_callback);
	survive_cal_install(priv->libsurvive_ctx);

	while(survive_poll(priv->libsurvive_ctx) == 0) {
	}
	return 0;
}

static ohmd_device* open_device(ohmd_driver* driver, ohmd_device_desc* desc)
{
	vive_priv* priv = ohmd_alloc(driver->ctx, sizeof(vive_priv));

	if(!priv)
		return NULL;

	priv->base.ctx = driver->ctx;

                                                  	priv->survive_poll_thread = ohmd_create_thread(driver->ctx, survive_poll_thread, priv);
	priv->survive_copy_mutex = ohmd_create_mutex(driver->ctx);
	//printf("thread %d creates libsurvive thread\n", pthread_self());

	// Set default device properties
	ohmd_set_default_device_properties(&priv->base.properties);

	// Set device properties TODO: Get from device
	priv->base.properties.hsize = 0.122822f;
	priv->base.properties.vsize = 0.068234f;
	priv->base.properties.hres = 2160;
	priv->base.properties.vres = 1200;
    /*
    // calculated from here: https://www.gamedev.net/topic/683698-projection-matrix-model-of-the-htc-vive/
	priv->base.properties.lens_sep = 0.057863;
	priv->base.properties.lens_vpos = 0.033896;
    */
    // estimated 'by eye' on jsarret's vive
	priv->base.properties.lens_sep = 0.056;
	priv->base.properties.lens_vpos = 0.032;
    float eye_to_screen_distance = 0.023226876441867737;
	//priv->base.properties.fov = DEG_TO_RAD(111.435f); //TODO: Confirm exact mesurements
	priv->base.properties.ratio = (2160.0f / 1200.0f) / 2.0f;

	/*
    ohmd_set_universal_distortion_k(&(priv->base.properties), 0.394119, -0.508383, 0.323322, 0.790942);
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
	priv->base.getf = getf;
	priv->base.close = close_device;

	return (ohmd_device*)priv;
}

static void get_device_list(ohmd_driver* driver, ohmd_device_list* list)
{
	struct hid_device_info* devs = hid_enumerate(HTC_ID, VIVE_HMD);
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
		desc->device_class = OHMD_DEVICE_CLASS_HMD;
		desc->device_flags =
		OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING |
		OHMD_DEVICE_FLAGS_ROTATIONAL_TRACKING |
		OHMD_DEVICE_FLAGS_LEFT_CONTROLLER |
		OHMD_DEVICE_FLAGS_RIGHT_CONTROLLER;

		cur_dev = cur_dev->next;
		idx++;
	}

	hid_free_enumeration(devs);
}

static void destroy_driver(ohmd_driver* drv)
{
	LOGD("shutting down HTC Vive driver");
	free(drv);
}

ohmd_driver* ohmd_create_htc_vive_drv(ohmd_context* ctx)
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
