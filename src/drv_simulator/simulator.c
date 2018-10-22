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
float hmdyaw = 0;
float hmdpitch = 0;
float hmdroll = 0;
float lcyaw = 0;
float lcpitch = 0;
float lcroll = 0;
float rcyaw = 0;
float rcpitch = 0;
float rcroll = 0;
#define CONTROLLER_VALUES 8
float lcontroller_values[CONTROLLER_VALUES] = { 0 };
float rcontroller_values[CONTROLLER_VALUES] = { 0 };
volatile bool gui_thread_started = false;

void destroy(GtkWidget* widget, gpointer data){
	gtk_main_quit();
}

void move(GtkRange *range, gpointer data){
	float *val = data;
	*val = gtk_range_get_value (range); // not entirely happy pointing directly to val in memory but ok
}

void addSlider(GtkWidget* box, GtkOrientation orientation, void* targetfunction, float* val, int max) {
	GtkWidget* slider = gtk_scale_new_with_range(orientation, -max, max, 0.1);
	gtk_range_set_value((GtkRange*) slider, *val); // set slider to value the variable initially has
	g_signal_connect (slider, "value-changed", G_CALLBACK (targetfunction), val);
	gtk_widget_show(slider);
	gtk_box_pack_start(GTK_BOX(box), slider, TRUE, TRUE, 0);
}

void press(GtkButton *btn, GdkEventButton *event, gpointer data){
	float *val = data;
	*val = 1;
}
void unpress(GtkButton *btn, GdkEventButton *event, gpointer data){
	float *val = data;
	*val = 0;
}

void addButton(GtkWidget* box, float* val, char* label) {
	GtkWidget* button = gtk_button_new_with_label(label);
	g_signal_connect (button, "button-press-event", G_CALLBACK (press), val);
	g_signal_connect (button, "button-release-event", G_CALLBACK (unpress), val);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);
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
		addSlider(hmdbox, GTK_ORIENTATION_HORIZONTAL, &move, &hmdpos.x, 10);
		addSlider(hmdbox, GTK_ORIENTATION_VERTICAL, &move, &hmdpos.y, 10);
		addSlider(hmdbox, GTK_ORIENTATION_VERTICAL, &move, &hmdpos.z, 10);
		addSlider(hmdbox, GTK_ORIENTATION_HORIZONTAL, &move, &hmdpitch, 360);
		addSlider(hmdbox, GTK_ORIENTATION_HORIZONTAL, &move, &hmdyaw, 360);
		addSlider(hmdbox, GTK_ORIENTATION_HORIZONTAL, &move, &hmdroll, 360);
		gtk_container_add(GTK_CONTAINER(mainhbox), hmdbox);

		GtkWidget* lcbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
		addSlider(lcbox, GTK_ORIENTATION_HORIZONTAL, &move, &lcpos.x, 10);
		addSlider(lcbox, GTK_ORIENTATION_VERTICAL, &move, &lcpos.y, 10);
		addSlider(lcbox, GTK_ORIENTATION_VERTICAL, &move, &lcpos.z, 10);
		addSlider(lcbox, GTK_ORIENTATION_HORIZONTAL, &move, &lcpitch, 360);
		addSlider(lcbox, GTK_ORIENTATION_HORIZONTAL, &move, &lcyaw, 360);
		addSlider(lcbox, GTK_ORIENTATION_HORIZONTAL, &move, &lcroll, 360);
		for (int i = 0; i < CONTROLLER_VALUES; i++) {
			char label[256];
			snprintf(label, 256, "button %d", i);
			addButton(lcbox, &lcontroller_values[i], label);
		}
		gtk_container_add(GTK_CONTAINER(mainhbox), lcbox);

		GtkWidget* rcbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
		addSlider(rcbox, GTK_ORIENTATION_HORIZONTAL, &move, &rcpos.x, 10);
		addSlider(rcbox, GTK_ORIENTATION_VERTICAL, &move, &rcpos.y, 10);
		addSlider(rcbox, GTK_ORIENTATION_VERTICAL, &move, &rcpos.z, 10);
		addSlider(rcbox, GTK_ORIENTATION_HORIZONTAL, &move, &rcpitch, 360);
		addSlider(rcbox, GTK_ORIENTATION_HORIZONTAL, &move, &rcyaw, 360);
		addSlider(rcbox, GTK_ORIENTATION_HORIZONTAL, &move, &rcroll, 360);
		for (int i = 0; i < CONTROLLER_VALUES; i++) {
			char label[256];
			snprintf(label, 256, "button %d", i);
			addButton(rcbox, &rcontroller_values[i], label);
		}
		gtk_container_add(GTK_CONTAINER(mainhbox), rcbox);

		gtk_widget_set_size_request(hmdbox, 200, 1);
		gtk_widget_set_size_request(lcbox, 200, 1);
		gtk_widget_set_size_request(rcbox, 200, 1);


		gtk_container_add(GTK_CONTAINER(window), mainhbox);
		gtk_widget_show_all(window);
		gtk_main();
	}
}

static void update_device(ohmd_device* device)
{
	simulator_priv* priv = (simulator_priv*)device;
}

int i = 0;
static int getf(ohmd_device* device, ohmd_float_value type, float* out)
{
	simulator_priv* priv = (simulator_priv*)device;

	long long now = timeInMilliseconds();

	switch(type){
	case OHMD_ROTATION_QUAT:
		// mind the order of rotation operations
		if (priv->id == 0) {
			//quatf yawq;
			quatf pitchq;
			quatf rollq;
			oquatf_from_angles(0, 1, 0, hmdyaw, (quatf*)out);
			oquatf_from_angles(1, 0, 0, hmdpitch, &pitchq);
			oquatf_from_angles(0, 0, 1, hmdroll, &rollq);

			oquatf_mult_me((quatf*) out, &pitchq);
			oquatf_mult_me((quatf*) out, &rollq);
			//oquatf_mult_me((quatf*) out, &yawq);

			//printf("quat %f %f %f %f\n", out[0], out[1], out[2], out[3]);

		} else if (priv->id == 1) {
			//quatf yawq;
			quatf pitchq;
			quatf rollq;
			oquatf_from_angles(0, 1, 0, lcyaw, (quatf*) out);
			oquatf_from_angles(1, 0, 0, lcpitch, &pitchq);
			oquatf_from_angles(0, 0, 1, lcroll, &rollq);

			oquatf_mult_me((quatf*) out, &pitchq);
			oquatf_mult_me((quatf*) out, &rollq);
		} else if (priv->id == 2) {
			//quatf yawq;
			quatf pitchq;
			quatf rollq;
			oquatf_from_angles(0, 1, 0, rcyaw, (quatf*)out);
			oquatf_from_angles(0, 0, 1, rcroll, &rollq);
			oquatf_from_angles(1, 0, 0, rcpitch, &pitchq);

			oquatf_mult_me((quatf*) out, &pitchq);
			oquatf_mult_me((quatf*) out, &rollq);
		} else {
			out[0] = 0;
			out[1] = 0;
			out[2] = 0;
			out[3] = 1;
		}
		break;

	case OHMD_POSITION_VECTOR:
		if(priv->id == 0){
			// HMD
			out[0] = hmdpos.x;
			out[1] = hmdpos.y;
			out[2] = hmdpos.z;
			//printf ("vec %f %f %f\n", out[0], out[1], out[2]);
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
		if(priv->id == 1) {
			memcpy(out, lcontroller_values, CONTROLLER_VALUES * sizeof(float));
			//printf("out %f\n", out[0]);
		} else if (priv->id == 2) {
			memcpy(out, rcontroller_values, CONTROLLER_VALUES * sizeof(float));
		}
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

	// calculate projection eye projection matrices from the device properties
	ohmd_calc_default_proj_matrices(&priv->base.properties);

	// set up device callbacks
	priv->base.update = update_device;
	priv->base.close = close_device;
	priv->base.getf = getf;

	lcpos.x = -0.5;
	lcpos.z = -0.5;
	rcpos.x = 0.5;
	rcpos.z = -0.5;

	if (!gui_thread_started) { // only create one gui thread, open gets called for hmd and each controller
		ohmd_thread* guithread = ohmd_create_thread(driver->ctx, (unsigned int (*)(void *))initgui, priv);
		gui_thread_started = true;
	}
	if (priv->id == 1 || priv->id == 2) {
		// both controllers have the same layout
		priv->base.properties.control_count = 8;
		priv->base.properties.controls_hints[0] = OHMD_ANALOG_PRESS;
		priv->base.properties.controls_hints[1] = OHMD_TRIGGER_CLICK;
		priv->base.properties.controls_hints[2] = OHMD_MENU;
		priv->base.properties.controls_hints[3] = OHMD_HOME;
		priv->base.properties.controls_hints[4] = OHMD_SQUEEZE;
		priv->base.properties.controls_hints[5] = OHMD_GENERIC; //touching the XY pad
		priv->base.properties.controls_hints[6] = OHMD_ANALOG_X;
		priv->base.properties.controls_hints[7] = OHMD_ANALOG_Y;
		priv->base.properties.controls_types[0] = OHMD_DIGITAL;
		priv->base.properties.controls_types[1] = OHMD_DIGITAL;
		priv->base.properties.controls_types[2] = OHMD_DIGITAL;
		priv->base.properties.controls_types[3] = OHMD_DIGITAL;
		priv->base.properties.controls_types[4] = OHMD_DIGITAL;
		priv->base.properties.controls_types[5] = OHMD_DIGITAL;
		priv->base.properties.controls_types[6] = OHMD_ANALOG;
		priv->base.properties.controls_types[7] = OHMD_ANALOG;
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

	desc->device_flags = OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING |
		OHMD_DEVICE_FLAGS_ROTATIONAL_TRACKING;
	desc->device_class = OHMD_DEVICE_CLASS_HMD;

	desc->id = id++;

	// Left Controller
	
	desc = &list->devices[list->num_devices++];

	strcpy(desc->driver, "OpenHMD Simulator Driver");
	strcpy(desc->vendor, "OpenHMD");
	strcpy(desc->product, "Left Controller Simulated Device");

	strcpy(desc->path, "(none)");

	desc->driver_ptr = driver;

	desc->device_flags =  OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING | 
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

	desc->device_flags = OHMD_DEVICE_FLAGS_POSITIONAL_TRACKING | 
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
