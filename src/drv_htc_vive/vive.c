/*
 * OpenHMD - Free and Open Source API and drivers for immersive technology.
 * Copyright (C) 2013 Fredrik Hultin.
 * Copyright (C) 2013 Jakob Bornecrantz.
 * Distributed under the Boost 1.0 licence, see LICENSE for full text.
 */

/* HTC Vive Driver */

#define FEATURE_BUFFER_SIZE 256

#define HTC_ID                   0x0bb4
#define VIVE_HMD                 0x2c87

#define VALVE_ID                 0x28de
#define VIVE_WATCHMAN_DONGLE     0x2101
#define VIVE_LIGHTHOUSE_FPGA_RX  0x2000

#define VIVE_TIME_DIV 48000000.0f

#include <string.h>
#include <wchar.h>
#include <hidapi.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

#include "vive.h"
#include <survive.h>
#include<pthread.h>

typedef struct {
	ohmd_device base;

	struct SurviveContext * ctx;

	vec3f pos;
	quatf rot;
	uint32_t last_ticks;
	uint8_t last_seq;

	vive_config_packet vive_config;
} vive_priv;

static void update_device(ohmd_device* device)
{
	vive_priv* priv = (vive_priv*)device;
	//printf("Update!\n");
	//survive_poll(priv->ctx);
}
FLT libsurvive_pos[3];
FLT libsurvive_quat[4];
static int getf(ohmd_device* device, ohmd_float_value type, float* out)
{
	vive_priv* priv = (vive_priv*)device;

	switch(type){
	case OHMD_ROTATION_QUAT:
		out[0] = priv->rot.w;
		out[1] = priv->rot.x;
		out[2] = priv->rot.y;
		out[3] = priv->rot.z;

		out[0] = libsurvive_quat[0];
		out[1] = libsurvive_quat[1];
		out[2] = libsurvive_quat[2];
		out[3] = libsurvive_quat[3];

		break;

	case OHMD_POSITION_VECTOR:
		out[0] = priv->pos.x;
		out[1] = priv->pos.y;
		out[2] = priv->pos.z;

		out[0] = libsurvive_pos[0];
		out[1] = libsurvive_pos[1];
		out[2] = libsurvive_pos[2];
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


void testprog_button_process(SurviveObject * so, uint8_t eventType, uint8_t buttonId, uint8_t axis1Id, uint16_t axis1Val, uint8_t axis2Id, uint16_t axis2Val)
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


void testprog_raw_pose_process(SurviveObject * so, uint8_t lighthouse, FLT *pos, FLT *quat)
{
	survive_default_raw_pose_process(so, lighthouse, pos, quat);

	// print the pose;
	if (strcmp(so->codename, "HMD") == 0 && lighthouse == 0) {
		for (int i = 0; i < 3; i++) {
			if (pos[i] < -10 || pos[i] > 10) return;
		}
		for (int i = 0; i < 4; i++) {
			if (quat[i] < -10 || quat[i] > 10) return;
		}
		printf("Pose: [%1.1x][%s][% 08.8f,% 08.8f,% 08.8f] [% 08.8f,% 08.8f,% 08.8f,% 08.8f]\n", lighthouse, so->codename, pos[0], pos[1], pos[2], quat[0], quat[1], quat[2], quat[3]);

		libsurvive_pos[0] = pos[0];
		libsurvive_pos[1] = pos[1];
		libsurvive_pos[2] = pos[2];

		libsurvive_quat[0] = quat[0];
		libsurvive_quat[1] = quat[1];
		libsurvive_quat[2] = quat[2];
		libsurvive_quat[3] = quat[3];
	}
}

void testprog_imu_process(SurviveObject * so, int mask, FLT * accelgyromag, uint32_t timecode, int id)
{
	survive_default_imu_process(so, mask, accelgyromag, timecode, id);
}

void* thread_func(void* argument) {
	//printf("My first thread!!!\n");
	while(survive_poll(((vive_priv*) argument)->ctx) == 0) {
	}
}

static ohmd_device* open_device(ohmd_driver* driver, ohmd_device_desc* desc)
{
	vive_priv* priv = ohmd_alloc(driver->ctx, sizeof(vive_priv));

	if(!priv)
		return NULL;

	int hret = 0;

	priv->base.ctx = driver->ctx;

	int idx = atoi(desc->path);

	/*
	dump_info_string(hid_get_manufacturer_string, "manufacturer", priv->hmd_handle);
	dump_info_string(hid_get_product_string , "product", priv->hmd_handle);
	dump_info_string(hid_get_serial_number_string, "serial number", priv->hmd_handle);
	*/

	// turn the display on
	//hret = hid_send_feature_report(priv->hmd_handle, vive_magic_power_on, sizeof(vive_magic_power_on));
	//printf("power on magic: %d\n", hret);

	//ASDF
	priv->ctx = survive_init( 0 );

	if( !priv->ctx )
	{
		fprintf( stderr, "Fatal. Could not start\n" );
	}


	survive_install_button_fn(priv->ctx, testprog_button_process);
	survive_install_raw_pose_fn(priv->ctx, testprog_raw_pose_process);
	survive_install_imu_fn(priv->ctx, testprog_imu_process);
	survive_cal_install(priv->ctx);

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
	priv->base.update = update_device;
	priv->base.getf = getf;

	//ofq_init(&priv->gyro_q, 128);

	pthread_t my_thread;
	pthread_create(&my_thread, NULL, thread_func, priv);

	return (ohmd_device*)priv;

cleanup:
	if(priv)
		free(priv);

	return NULL;
}

static void get_device_list(ohmd_driver* driver, ohmd_device_list* list)
{
	struct hid_device_info* devs = hid_enumerate(HTC_ID, VIVE_HMD);
	struct hid_device_info* cur_dev = devs;

	int idx = 0;
	while (cur_dev) {
		ohmd_device_desc* desc = &list->devices[list->num_devices++];

		strcpy(desc->driver, "OpenHMD HTC Vive Driver");
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
