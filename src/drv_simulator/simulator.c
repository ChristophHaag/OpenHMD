/*
 * OpenHMD - Free and Open Source API and drivers for immersive technology.
 * Copyright (C) 2013 Fredrik Hultin.
 * Copyright (C) 2013 Jakob Bornecrantz.
 * Distributed under the Boost 1.0 licence, see LICENSE for full text.
 */

/* Dummy Driver */

#include <string.h>
#include "../openhmdi.h"
#include <sys/time.h>

long long timeInMilliseconds(void) {
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

typedef struct {
	ohmd_device base;
	int id;

	bool move_hmd;
	bool rotate_hmd;
	bool move_lcontroller;
	bool move_rcontroller;

	vec3f current_hmd_pos;
	bool hmd_forward;
	long long last_hmd_pos_update;
	long long last_hmd_rotation_update;
	double current_hmd_y_angle;

	vec3f current_lcontroller_pos;
	bool lcontroller_left;
	long long last_lcontroller_pos_update;
} simulator_priv;

static void update_device(ohmd_device* device)
{
	simulator_priv* priv = (simulator_priv*)device;
}

static int getf(ohmd_device* device, ohmd_float_value type, float* out)
{
	simulator_priv* priv = (simulator_priv*)device;

	long long now = timeInMilliseconds();
	double sec;
	switch(type){
	case OHMD_ROTATION_QUAT:
		sec = (now - priv->last_hmd_rotation_update) / 1000.;
		priv->last_hmd_rotation_update = now;
		priv->current_hmd_y_angle += 10*sec; // rotate 10°/second
		if (priv->rotate_hmd)
			oquatf_from_angles(0, 1, 0, priv->current_hmd_y_angle, (quatf*) out);
		break;

	case OHMD_POSITION_VECTOR:
		if(priv->id == 0){
			// HMD
			sec = (now - priv->last_hmd_pos_update) / 1000.;
			priv->last_hmd_pos_update = now;
			if (priv->move_hmd)
				priv->current_hmd_pos.z = priv->current_hmd_pos.z + (priv->hmd_forward ? sec / 2.: -sec / 2. );
			//printf("%f %f \n", sec, priv->current_pos.z);
			if (fabs(priv->current_hmd_pos.z) > 0.7) priv->hmd_forward = !priv->hmd_forward;
			out[0] = priv->current_hmd_pos.x;
			out[1] = priv->current_hmd_pos.y;
			out[2] = priv->current_hmd_pos.z;
		}
		else if(priv->id == 1)
		{
			sec = (now - priv->last_lcontroller_pos_update) / 1000.;
			priv->last_lcontroller_pos_update = now;
			if (priv->move_lcontroller)
				priv->current_lcontroller_pos.x = priv->current_lcontroller_pos.z + (priv->lcontroller_left ? sec / 2.: -sec / 2. );
			//printf("%f %f \n", sec, priv->current_pos.z);
			if (fabs(priv->current_lcontroller_pos.x) > 0.3) priv->lcontroller_left= !priv->lcontroller_left;

			// Left Controller
			out[0] = priv->current_lcontroller_pos.x;
			out[1] = priv->current_lcontroller_pos.y;
			out[2] = priv->current_lcontroller_pos.z;
		}
		else
		{
			// Right Controller
			out[0] = .5f;
			out[1] = 0;
			out[2] = 0;
		}
		break;

	case OHMD_DISTORTION_K:
		// TODO this should be set to the equivalent of no distortion
		memset(out, 0, sizeof(float) * 6);
		break;
	
	case OHMD_CONTROLS_STATE:
		out[0] = .1f;
		out[1] = 1.0f;
		break;

	default:
		ohmd_set_error(priv->base.ctx, "invalid type given to getf (%ud)", type);
		return OHMD_S_INVALID_PARAMETER;
		break;
	}

	return OHMD_S_OK;
}

static void close_device(ohmd_device* device)
{
	LOGD("closing dummy device");
	free(device);
}

static ohmd_device* open_device(ohmd_driver* driver, ohmd_device_desc* desc)
{
	simulator_priv* priv = ohmd_alloc(driver->ctx, sizeof(simulator_priv));
	if(!priv)
		return NULL;
	
	priv->id = desc->id;
	
	// Set default device properties
	ohmd_set_default_device_properties(&priv->base.properties);

	// Set device properties (imitates the rift values)
	priv->base.properties.hsize = 0.149760f;
	priv->base.properties.vsize = 0.093600f;
	priv->base.properties.hres = 1280;
	priv->base.properties.vres = 800;
	priv->base.properties.lens_sep = 0.063500f;
	priv->base.properties.lens_vpos = 0.046800f;
	priv->base.properties.fov = DEG_TO_RAD(125.5144f);
	priv->base.properties.ratio = (1280.0f / 800.0f) / 2.0f;
	
	// Some buttons and axes
	priv->base.properties.control_count = 2;
	priv->base.properties.controls_hints[0] = OHMD_BUTTON_A;
	priv->base.properties.controls_hints[1] = OHMD_MENU;
	priv->base.properties.controls_types[0] = OHMD_ANALOG;
	priv->base.properties.controls_types[1] = OHMD_DIGITAL;

	// calculate projection eye projection matrices from the device properties
	ohmd_calc_default_proj_matrices(&priv->base.properties);

	// set up device callbacks
	priv->base.update = update_device;
	priv->base.close = close_device;
	priv->base.getf = getf;


	//priv->current_quat.z = 1;
	priv->current_hmd_pos.x = 0;
	priv->current_hmd_pos.y = 0;
	priv->current_hmd_pos.z = 0.5;
	priv->last_hmd_pos_update = timeInMilliseconds();
	priv->last_hmd_rotation_update = timeInMilliseconds();


	priv->current_lcontroller_pos.x = -0.5;
	priv->current_lcontroller_pos.y = 0;
	priv->current_lcontroller_pos.z = 0;

	priv->move_hmd = false;
	priv->rotate_hmd = false;
	priv->move_lcontroller = true;
	priv->move_rcontroller = true;

	return (ohmd_device*)priv;
}

static void get_device_list(ohmd_driver* driver, ohmd_device_list* list)
{
	int id = 0;
	ohmd_device_desc* desc;

	// HMD

	desc = &list->devices[list->num_devices++];

	strcpy(desc->driver, "OpenHMD Simulator Driver");
	strcpy(desc->vendor, "OpenHMD");
	strcpy(desc->product, "HMD Simulated Device");

	strcpy(desc->path, "(none)");

	desc->driver_ptr = driver;

	desc->device_flags = OHMD_DEVICE_FLAGS_NULL_DEVICE | OHMD_DEVICE_FLAGS_ROTATIONAL_TRACKING;
	desc->device_class = OHMD_DEVICE_CLASS_HMD;

	desc->id = id++;

	// Left Controller
	
	desc = &list->devices[list->num_devices++];

	strcpy(desc->driver, "OpenHMD Simulator Driver");
	strcpy(desc->vendor, "OpenHMD");
	strcpy(desc->product, "Left Controller Simulated Device");

	strcpy(desc->path, "(none)");

	desc->driver_ptr = driver;

	desc->device_flags = OHMD_DEVICE_FLAGS_NULL_DEVICE | 
		OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING | 
		OHMD_DEVICE_FLAGS_ROTATIONAL_TRACKING | 
		OHMD_DEVICE_FLAGS_LEFT_CONTROLLER;

	desc->device_class = OHMD_DEVICE_CLASS_CONTROLLER;

	desc->id = id++;
	
	// Right Controller
	
	desc = &list->devices[list->num_devices++];

	strcpy(desc->driver, "OpenHMD Simulator Driver");
	strcpy(desc->vendor, "OpenHMD");
	strcpy(desc->product, "Right Controller Simulated Device");

	strcpy(desc->path, "(none)");

	desc->driver_ptr = driver;

	desc->device_flags = OHMD_DEVICE_FLAGS_NULL_DEVICE | 
		OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING | 
		OHMD_DEVICE_FLAGS_ROTATIONAL_TRACKING | 
		OHMD_DEVICE_FLAGS_RIGHT_CONTROLLER;

	desc->device_class = OHMD_DEVICE_CLASS_CONTROLLER;

	desc->id = id++;
}

static void destroy_driver(ohmd_driver* drv)
{
	LOGD("shutting down dummy driver");
	free(drv);
}

ohmd_driver* ohmd_create_simulator_drv(ohmd_context* ctx)
{
	ohmd_driver* drv = ohmd_alloc(ctx, sizeof(ohmd_driver));
	if(!drv)
		return NULL;

	drv->get_device_list = get_device_list;
	drv->open_device = open_device;
	drv->get_device_list = get_device_list;
	drv->open_device = open_device;
	drv->destroy = destroy_driver;

	return drv;
}
