#include "simplerenderer.h"
#include <stdbool.h>

static void compile_shader_src(GLuint shader, const char* src)
{
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if(status == GL_FALSE){
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		char log[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, &infoLogLength, log);
		printf("compile failed: %s\n", log);
	}
}

static GLuint compile_shader(const char* vertex, const char* fragment)
{
	// Create the handles
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint programShader = glCreateProgram();

	// Attach the shaders to a program handle.
	glAttachShader(programShader, vertexShader);
	glAttachShader(programShader, fragmentShader);

	// Load and compile the Vertex Shader
	compile_shader_src(vertexShader, vertex);

	// Load and compile the Fragment Shader
	compile_shader_src(fragmentShader, fragment);

	// The shader objects are not needed any more,
	// the programShader is the complete shader to be used.
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glLinkProgram(programShader);

	GLint status;
	glGetProgramiv(programShader, GL_LINK_STATUS, &status);

	if(status == GL_FALSE){
		GLint infoLogLength;
		glGetProgramiv(programShader, GL_INFO_LOG_LENGTH, &infoLogLength);
		char log[infoLogLength + 1];
		glGetProgramInfoLog(programShader, infoLogLength, &infoLogLength, log);
		printf("compile failed: %s\n", log);
	}
	return programShader;
}

SimpleRenderer *
openhmd_create_simple_renderer(ohmd_device *device, SDL_GLContext appcontext, SDL_Window *appwindow)
{
	// TODO: rotated screeens
	int hmd_w, hmd_h;
	ohmd_device_geti(device, OHMD_SCREEN_HORIZONTAL_RESOLUTION, &hmd_w);
	ohmd_device_geti(device, OHMD_SCREEN_VERTICAL_RESOLUTION, &hmd_h);

	SimpleRenderer *renderer = calloc(1, sizeof(SimpleRenderer));
	renderer->device = device;

	if (!appcontext || !appwindow) {
		printf("ERROR: Simple Renderer only works with applications that run on "
		"an OpenGL context created with SDL!\n");
		free(renderer);
		return NULL;
	}

	renderer->applicationglcontext = appcontext;
	renderer->applicationwindow = appwindow;
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	// TODO: only works with SDL apps
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1); 

	renderer->window = SDL_CreateWindow("OpenHMD simple renderer",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			hmd_w, hmd_h, SDL_WINDOW_OPENGL );
	if(renderer->window == NULL) {
		printf("SDL_CreateWindow failed\n");
		free(renderer);
		return NULL;
	}
	renderer->w = hmd_w;
	renderer->h = hmd_h;
	renderer->is_fullscreen = 0;

	renderer->glcontext = SDL_GL_CreateContext(renderer->window);
	if(renderer->glcontext == NULL){
		printf("SDL_GL_CreateContext\n");
		free(renderer);
		return NULL;
	}

	SDL_GL_SetSwapInterval(1);
	
	printf("Simple Renderer using OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Simple Renderer using OpenGL Version: %s\n", glGetString(GL_VERSION)); 

	
	ohmd_device_getf(device, OHMD_EYE_IPD, &renderer->ipd);
	//viewport is half the screen
	ohmd_device_getf(device, OHMD_SCREEN_HORIZONTAL_SIZE, &(renderer->viewport_scale[0]));
	renderer->viewport_scale[0] /= 2.0f;
	ohmd_device_getf(device, OHMD_SCREEN_VERTICAL_SIZE, &(renderer->viewport_scale[1]));
	//distortion coefficients
	ohmd_device_getf(device, OHMD_UNIVERSAL_DISTORTION_K, &(renderer->distortion_coeffs[0]));
	ohmd_device_getf(device, OHMD_UNIVERSAL_ABERRATION_K, &(renderer->aberr_scale[0]));
	//calculate lens centers (assuming the eye separation is the distance between the lens centers)
	ohmd_device_getf(device, OHMD_LENS_HORIZONTAL_SEPARATION, &renderer->sep);
	ohmd_device_getf(device, OHMD_LENS_VERTICAL_POSITION, &(renderer->left_lens_center[1]));
	ohmd_device_getf(device, OHMD_LENS_VERTICAL_POSITION, &(renderer->right_lens_center[1]));
	renderer->left_lens_center[0] = renderer->viewport_scale[0] - renderer->sep/2.0f;
	renderer->right_lens_center[0] = renderer->sep/2.0f;
	//assume calibration was for lens view to which ever edge of screen is further away from lens center
	renderer->warp_scale = (renderer->left_lens_center[0] > renderer->right_lens_center[0]) ?
		renderer->left_lens_center[0] : renderer->right_lens_center[0];
	renderer->warp_adj = 1.0f;

	
	// One big triangle that covers the screen space from -1,-1 to 1,1.
	// Just like a quad made out of two smaller triangles would.
    renderer->quadvertices[0] = -1;
	renderer->quadvertices[1] = 3;
	renderer->quadvertices[2] = -1;
	renderer->quadvertices[3] = -1;
	renderer->quadvertices[4] = 3;
	renderer->quadvertices[5] = -1;
	
	const char* vertex;
	ohmd_gets(OHMD_GLSL_330_DISTORTION_VERT_SRC, &vertex);
	const char* fragment;
	ohmd_gets(OHMD_GLSL_330_DISTORTION_FRAG_SRC, &fragment);
	renderer->distortionshader = compile_shader(vertex, fragment);
	
	GLuint distortionVBO[1];
	glGenBuffers(1, distortionVBO);

	glGenVertexArrays(1, &renderer->distortionVAO[0]);

	glBindVertexArray(renderer->distortionVAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, distortionVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(renderer->quadvertices), renderer->quadvertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	
	SDL_GL_MakeCurrent(renderer->applicationwindow, renderer->applicationglcontext);
	return renderer;
}

void
openhmd_render_companion(SimpleRenderer *renderer, GLuint tex)
{
	glBindTexture(GL_TEXTURE_2D, tex);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, renderer->w, renderer->h, 0, 0, renderer->w, renderer->h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	SDL_GL_SwapWindow(renderer->applicationwindow);
}

void
openhmd_render_textures(SimpleRenderer *renderer, GLuint left, GLuint right)
{
	SDL_GL_MakeCurrent(renderer->window, renderer->glcontext);

	// draw the textures to the screen, applying the distortion shader
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glUseProgram(renderer->distortionshader);
	glUniform1i(glGetUniformLocation(renderer->distortionshader, "warpTexture"), 0);
	glUniform2fv(glGetUniformLocation(renderer->distortionshader, "ViewportScale"), 1, renderer->viewport_scale);
	glUniform3fv(glGetUniformLocation(renderer->distortionshader, "aberr"), 1, renderer->aberr_scale);
	glUniform1f(glGetUniformLocation(renderer->distortionshader, "WarpScale"), renderer->warp_scale*renderer->warp_adj);
	glUniform4fv(glGetUniformLocation(renderer->distortionshader, "HmdWarpParam"), 1, renderer->distortion_coeffs);
	// The shader is set up to render starting at the middle of the viewport
	// and half its size. Move it to the bottom left and double its size.
	float mvp[16] = {
		2.0, 0.0, 0.0, 0.0,
		0.0, 2.0, 0.0, 0.0,
		0.0, 0.0, 2.0, 0.0,
		-1.0, -1.0, 0.0, 1.0
	};
	glUniformMatrix4fv(glGetUniformLocation(renderer->distortionshader, "mvp"), 1, GL_FALSE, mvp);

	for (int i = 0; i < 2; i++)
	{
		if (i == 0) {
			glViewport(0, 0, renderer->w / 2, renderer->h);
			glScissor(0, 0, renderer->w / 2, renderer->h);
			glClearColor(0.5, 0, 0.0, 1.0);
		} else {
			glViewport(renderer->w / 2, 0, renderer->w / 2, renderer->h);
			glScissor(renderer->w / 2, 0, renderer->w / 2, renderer->h);
			glClearColor(0, 0.5, 0.0, 1.0);
		}
		// TODO: Why does this clear both glViewports?
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glUniform2fv(glGetUniformLocation(renderer->distortionshader, "LensCenter"), 1, i == 0 ? renderer->left_lens_center : renderer->right_lens_center);

		glBindVertexArray(renderer->distortionVAO[0]);

		//glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, i == 0 ? left : right);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	// Da swap-dawup!
	SDL_GL_SwapWindow(renderer->window);
	
	SDL_GL_MakeCurrent(renderer->applicationwindow, renderer->applicationglcontext);
}
