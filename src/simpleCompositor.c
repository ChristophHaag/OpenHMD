

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1

#include <stdio.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <openhmd.h>
#include "shaders.h"
#include "simpleCompositor.h"

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

int initCompositor(int hmd_index) {

	ohmd_context* ctx = ohmd_ctx_create();
	int num_devices = ohmd_ctx_probe(ctx);
	if(num_devices < 0){
		printf("failed to probe devices: %s\n", ohmd_ctx_get_error(ctx));
		return 1;
	}
	ohmd_device_settings* settings = ohmd_device_settings_create(ctx);


	ohmd_device* hmd = ohmd_list_open_device_s(ctx, hmd_index, settings);
	ohmd_device_geti(hmd, OHMD_SCREEN_HORIZONTAL_RESOLUTION, &hmd_w);
	ohmd_device_geti(hmd, OHMD_SCREEN_VERTICAL_RESOLUTION, &hmd_h);
	float ipd;
	ohmd_device_getf(hmd, OHMD_EYE_IPD, &ipd);
	float viewport_scale[2];
	float distortion_coeffs[4];
	float aberr_scale[3];
	float sep;
	//viewport is half the screen
	ohmd_device_getf(hmd, OHMD_SCREEN_HORIZONTAL_SIZE, &(viewport_scale[0]));
	viewport_scale[0] /= 2.0f;
	ohmd_device_getf(hmd, OHMD_SCREEN_VERTICAL_SIZE, &(viewport_scale[1]));
	//distortion coefficients
	ohmd_device_getf(hmd, OHMD_UNIVERSAL_DISTORTION_K, &(distortion_coeffs[0]));
	ohmd_device_getf(hmd, OHMD_UNIVERSAL_ABERRATION_K, &(aberr_scale[0]));
	//calculate lens centers (assuming the eye separation is the distance between the lens centers)
	ohmd_device_getf(hmd, OHMD_LENS_HORIZONTAL_SEPARATION, &sep);
	ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(left_lens_center[1]));
	ohmd_device_getf(hmd, OHMD_LENS_VERTICAL_POSITION, &(right_lens_center[1]));
	left_lens_center[0] = viewport_scale[0] - sep/2.0f;
	right_lens_center[0] = sep/2.0f;
	//assume calibration was for lens view to which ever edge of screen is further away from lens center
	float warp_scale = (left_lens_center[0] > right_lens_center[0]) ? left_lens_center[0] : right_lens_center[0];
	float warp_adj = 1.0f;
	ohmd_close_device(hmd); // so the application can use it

	glUniform1i(glGetUniformLocation(shaderProgramID, "warpTexture"), 0);
	glUniform2fv(glGetUniformLocation(shaderProgramID, "ViewportScale"), 1, viewport_scale);
	glUniform3fv(glGetUniformLocation(shaderProgramID, "aberr"), 1, aberr_scale);

	glUniform1f(glGetUniformLocation(shaderProgramID, "WarpScale"), warp_scale*warp_adj);
	glUniform4fv(glGetUniformLocation(shaderProgramID, "HmdWarpParam"), 1, distortion_coeffs);

	eye_w = hmd_w/2;
	eye_h = hmd_h;


	glXCreateContextAttribsARBProc glXCreateContextAttribsARB  = (glXCreateContextAttribsARBProc) glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

	display = XOpenDisplay(NULL);
	if (!display)
	{
		printf("Failed to open X display\n");
		return 1;
	}
	// Get a matching FB config
	static int visual_attribs[] =
	{
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_ALPHA_SIZE      , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_STENCIL_SIZE    , 8,
		GLX_DOUBLEBUFFER    , True,
		//GLX_SAMPLE_BUFFERS  , 1,
		//GLX_SAMPLES         , 4,
		None
	};

	int fbcount;
	GLXFBConfig* fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
	if (!fbc)
	{
		printf( "Failed to retrieve a framebuffer config\n" );
		return 1;
	}
	printf( "Found %d matching FB configs.\n", fbcount );
	// Pick the FB config/visual with the most samples per pixel
	printf( "Getting XVisualInfos\n" );
	int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

	int i;
	for (i=0; i<fbcount; ++i)
	{
		XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
		if ( vi )
		{
			int samp_buf, samples;
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
			glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLES       , &samples  );

			printf( "  Matching fbconfig %d, visual ID 0x%2lx: SAMPLE_BUFFERS = %d,"
			" SAMPLES = %d\n",
	   i, vi -> visualid, samp_buf, samples );

			if ( best_fbc < 0 || (samp_buf && samples) > best_num_samp )
				best_fbc = i, best_num_samp = samples;
			if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
				worst_fbc = i, worst_num_samp = samples;
		}
		XFree( vi );
	}

	GLXFBConfig bestFbc = fbc[ best_fbc ];

	// Be sure to free the FBConfig list allocated by glXChooseFBConfig()
	XFree( fbc );

	// Get a visual
	XVisualInfo *vi = glXGetVisualFromFBConfig( display, bestFbc );
	printf( "Chosen visual ID = 0x%lx\n", vi->visualid );

	printf( "Creating colormap\n" );
	XSetWindowAttributes swa;
	Colormap cmap;
	swa.colormap = cmap = XCreateColormap( display,
					       RootWindow( display, vi->screen ),
					       vi->visual, AllocNone );
	swa.background_pixmap = None ;
	swa.border_pixel      = 0;
	swa.event_mask        = StructureNotifyMask;

	printf( "Creating window\n" );
	win = XCreateWindow( display, RootWindow( display, vi->screen ),
				    0, 0, hmd_w, hmd_h, 0, vi->depth, InputOutput,
			     vi->visual,
			     CWBorderPixel|CWColormap|CWEventMask, &swa );
	if ( !win )
	{
		printf( "Failed to create window.\n" );
		return 1;
	}

	// Done with the visual info data
	XFree( vi );

	XStoreName( display, win, "GL 3.0 Window" );

	printf( "Mapping window\n" );
	XMapWindow( display, win );

	int context_attribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 0,
		//GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		None
	};

	appctx = glXGetCurrentContext();
	if (!appctx) {
		printf("Application doesn't have context!\n");
		//return 1;
	}

	printf( "Creating context and sharing with application context\n" );
	GLXContext glxctx = glXCreateContextAttribsARB( display, bestFbc, appctx,
						True, context_attribs );

	printf( "Making compositor context current\n" );
	glXMakeCurrent( display, win, glxctx );

	glClearColor( 0, 0.5, 1, 1 );
	glClear( GL_COLOR_BUFFER_BIT );
	glXSwapBuffers ( display, win );

	const char* vertex;
	ohmd_gets(OHMD_GLSL_330_DISTORTION_VERT_SRC, &vertex);
	const char* fragment;
	ohmd_gets(OHMD_GLSL_330_DISTORTION_FRAG_SRC, &fragment);


	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderId, 1, &vertex, NULL);
	glCompileShader(vertexShaderId);
	int vertexcompilesuccess;
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &vertexcompilesuccess);
	if (!vertexcompilesuccess) {
		char infoLog[512];
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		printf("Vertex Shader failed to compile: %s\n", infoLog);
		return -1;
	} else {
		printf("Successfully compiled vertex shader!\n");
	}

	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderId, 1, &fragment, NULL);
	glCompileShader(fragmentShaderId);
	int fragmentcompilesuccess;
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &fragmentcompilesuccess);
	if (!fragmentcompilesuccess) {
		char infoLog[512];
		glGetShaderInfoLog(fragmentShaderId, 512, NULL, infoLog);
		printf("Fragment Shader failed to compile: %s\n", infoLog);
		return -1;
	} else  {
		printf("Successfully compiled fragment shader!\n");
	}

	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShaderId);
	glAttachShader(shaderProgramID, fragmentShaderId);
	glLinkProgram(shaderProgramID);
	GLint shaderprogramsuccess;
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &shaderprogramsuccess);
	if (!shaderprogramsuccess) {
		char infoLog[512];
		glGetProgramInfoLog(shaderProgramID, 512, NULL, infoLog);
		printf("Shader Program failed to link: %s\n", infoLog);
	} else {
		printf("Successfully linked shader program!\n");
	}

	glDeleteShader(vertexShaderId);
	glDeleteShader(fragmentShaderId);


	glGenTextures(1, &left_color_tex);
	glGenTextures(1, &left_depth_tex);
	glGenFramebuffers(1, &left_fbo);

	glBindTexture(GL_TEXTURE_2D, left_color_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, eye_w, eye_h, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, left_depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, eye_w, eye_h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, left_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, left_color_tex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, left_depth_tex, 0);

	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

	if(status != GL_FRAMEBUFFER_COMPLETE_EXT){
		printf("failed to create fbo %x\n", status);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);




	glGenTextures(1, &right_color_tex);
	glGenTextures(1, &right_depth_tex);
	glGenFramebuffers(1, &right_fbo);

	glBindTexture(GL_TEXTURE_2D, right_color_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, eye_w, eye_h, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, right_depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, eye_w, eye_h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, right_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, right_color_tex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, right_depth_tex, 0);

	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

	if(status != GL_FRAMEBUFFER_COMPLETE_EXT){
		printf("failed to create fbo %x\n", status);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	float vertices[] = {
		-1, 1, 0.0f,
		-1, -1, 0.0f,
		1,  -1, 0.0f,

		-1, 1, 0.0f,
		1, 1, 0.0f,
		1,  -1, 0.0f
	};

	unsigned int VBO;
	glGenBuffers(1, &VBO);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	return 0;

}

void submitFrame() {

	glViewport(0, 0, hmd_w, hmd_h);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	for (int eye = 0; eye < 2; eye++) {
		float m[4][4];

		// calculate an MVP that covers the correct side of the window
		m[0][0] = 1.0;
		m[0][1] = 0.0;
		m[0][2] = 0.0;
		m[0][3] = 0.0;
		m[1][0] = 0.0;
		m[1][1] = 2.0;
		m[1][2] = 0.0;
		m[1][3] = 0.0;
		m[2][0] = 0.0;
		m[2][1] = 0.0;
		m[2][2] = 0.0;
		m[2][3] = 0.0;
		if (eye == 0) {
			m[3][0] = -1.0;
			m[3][1] = -1.0;
			m[3][2] = 0.0;
			m[3][3] = 1.0;
		} else {
			m[3][0] = 0.0;
			m[3][1] = -1.0;
			m[3][2] = 0.0;
			m[3][3] = 1.0;
		};

		// set our shader up
		glUseProgram(shaderProgramID);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgramID, "mvp"), 1, GL_FALSE, (const float *)m);
		glUniform2fv(glGetUniformLocation(shaderProgramID, "LensCenter"), 1, eye == 0 ? left_lens_center : right_lens_center);

		// set our texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, eye == 0 ? left_color_tex : right_color_tex);

		// bind our vao to restore our state
		glBindVertexArray(VAO);

		// render our rectangle
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDrawArrays(GL_TRIANGLES, 3, 3);
	}
	glXSwapBuffers(display, win);
}

void tearDown(Display* display, GLXContext ctx, GLXDrawable win, Colormap cmap) {
	glXMakeCurrent( display, 0, 0 );
	glXDestroyContext( display, ctx );

	XDestroyWindow( display, win );
	XFreeColormap( display, cmap );
	XCloseDisplay( display );
}
