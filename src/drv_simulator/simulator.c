/*
 * OpenHMD - Free and Open Source API and drivers for immersive technology.
 * Copyright (C) 2013 Fredrik Hultin.
 * Copyright (C) 2013 Jakob Bornecrantz.
 * Distributed under the Boost 1.0 licence, see LICENSE for full text.
 */

/* Simulator Driver */

#include <gtk/gtk.h>

#include <string.h>
#include "../openhmdi.h"
#include <sys/time.h>

long long timeInMilliseconds(void) {
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

typedef struct simulator_priv {
	ohmd_device base;
	int id;
} simulator_priv;

//TODO: not global
quatf hmdrot;
vec3f hmdpos;
quatf lcrot;
vec3f lcpos;
quatf rcrot;
vec3f rcpos;

void destroy(GtkWidget* widget, gpointer data){
	gtk_main_quit();
}

void move(GtkRange *range, gpointer data){
	float *val = data;
	printf("move %p: %f\n", val, *val);
	*val = gtk_range_get_value (range); // not entirely happy pointing directly to val in memory but ok
}

void addSlider(GtkWidget* box, GtkOrientation orientation, void* targetfunction, float* val) {
	int movemax = 10;
	printf("Addslider %p\n", val);
	GtkWidget* slider = gtk_scale_new_with_range(orientation, -movemax, movemax, 0.1);
	gtk_range_set_value((GtkRange*) slider, 0);
	g_signal_connect (slider, "value-changed", G_CALLBACK (targetfunction), val);
	gtk_widget_show(slider);
	gtk_box_pack_start(GTK_BOX(box), slider, TRUE, TRUE, 0);
}

void initgui(simulator_priv* priv) {
	g_usleep(100000); // wait 100ms before opening the gui. Prevents X freeze. Why?
	while (true) { // reopen on close
		gtk_init(0, 0);
		GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_default_size(GTK_WINDOW(window), 600, 600);

		g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);
		gtk_container_set_border_width(GTK_CONTAINER(window), 10);
		gtk_window_set_title(GTK_WINDOW(window), "OpenHMD Simulator Controls");

		GtkWidget* mainhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

		GtkWidget* hmdbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
		addSlider(hmdbox, GTK_ORIENTATION_HORIZONTAL, &move, &hmdpos.x);
		addSlider(hmdbox, GTK_ORIENTATION_VERTICAL, &move, &hmdpos.y);
		addSlider(hmdbox, GTK_ORIENTATION_VERTICAL, &move, &hmdpos.z);

		GtkWidget* lcbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
		addSlider(lcbox, GTK_ORIENTATION_HORIZONTAL, &move, &lcpos.x);
		addSlider(lcbox, GTK_ORIENTATION_VERTICAL, &move, &lcpos.y);
		addSlider(lcbox, GTK_ORIENTATION_VERTICAL, &move, &lcpos.z);

		GtkWidget* rcbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
		addSlider(rcbox, GTK_ORIENTATION_HORIZONTAL, &move, &rcpos.x);
		addSlider(rcbox, GTK_ORIENTATION_VERTICAL, &move, &rcpos.y);
		addSlider(rcbox, GTK_ORIENTATION_VERTICAL, &move, &rcpos.z);

		gtk_widget_set_size_request(hmdbox, 200, 1);
		gtk_widget_set_size_request(lcbox, 200, 1);
		gtk_widget_set_size_request(rcbox, 200, 1);


		gtk_container_add(GTK_CONTAINER(mainhbox), hmdbox);
		gtk_container_add(GTK_CONTAINER(mainhbox), lcbox);
		gtk_container_add(GTK_CONTAINER(mainhbox), rcbox);
		gtk_container_add(GTK_CONTAINER(window), mainhbox);
		gtk_widget_show_all(window);
		gtk_main();
	}
}

static void update_device(ohmd_device* device)
{
	simulator_priv* priv = (simulator_priv*)device;
}

static int getf(ohmd_device* device, ohmd_float_value type, float* out)
{
	simulator_priv* priv = (simulator_priv*)device;

	long long now = timeInMilliseconds();

	switch(type){
	case OHMD_ROTATION_QUAT:
		break;

	case OHMD_POSITION_VECTOR:
		if(priv->id == 0){
			// HMD
			out[0] = hmdpos.x;
			out[1] = hmdpos.y;
			out[2] = hmdpos.z;
		}
		else if(priv->id == 1)
		{
			// Left Controller
			out[0] = lcpos.x;
			out[1] = lcpos.y;
			out[2] = lcpos.z;
		}
		else
		{
			// Right Controller
			out[0] = rcpos.x;
			out[1] = rcpos.y;
			out[2] = rcpos.z;
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

	if (priv->id == 0) { // only create one gui thread, open gets called for hmd and each controller
		ohmd_thread* guithread = ohmd_create_thread(driver->ctx, (unsigned int (*)(void *))initgui, priv);
	}

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
