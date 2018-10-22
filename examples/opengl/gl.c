/*
 * OpenHMD - Free and Open Source API and drivers for immersive technology.
 * Copyright (C) 2013 Fredrik Hultin.
 * Copyright (C) 2013 Jakob Bornecrantz.
 * Distributed under the Boost 1.0 licence, see LICENSE for full text.
 */

/* OpenGL Test - GL Helper Functions Implementation */

#include "gl.h"
#include <string.h>
#include <math.h>

#ifdef __unix
#include <signal.h>
#endif

static char* vertexshader =
"#version 450 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 2) uniform mat4 model;\n"
"layout(location = 3) uniform mat4 view;\n"
"layout(location = 4) uniform mat4 proj;\n"
"out vec2 vertexColor;\n"
"void main() {\n"
"	gl_Position = proj * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\n";

static char* fragmentshader =
"#version 450 core\n"
"layout(location = 0) out vec4 FragColor;\n"
"layout(location = 1) uniform vec4 uniformColor;\n"
"in vec2 vertexColor;\n"
"void main() {\n"
"	FragColor = vec4(uniformColor);\n"
"}\n";

void init_gl(gl_ctx* ctx, int w, int h, GLuint *VAOs, GLuint *appshader)
{
	memset(ctx, 0, sizeof(gl_ctx));

	// == Initialize SDL ==
	int ret = SDL_Init(SDL_INIT_EVERYTHING);
	if(ret < 0){
		printf("SDL_Init failed\n");
		exit(-1);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	ctx->window = SDL_CreateWindow("OpenHMD opengl example",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			w, h, SDL_WINDOW_OPENGL );
	if(ctx->window == NULL) {
		printf("SDL_CreateWindow failed\n");
		exit(-1);
	}
	ctx->w = w;
	ctx->h = h;
	ctx->is_fullscreen = 0;

	ctx->glcontext = SDL_GL_CreateContext(ctx->window);
	if(ctx->glcontext == NULL){
		printf("SDL_GL_CreateContext\n");
		exit(-1);
	}

	SDL_GL_SetSwapInterval(1);

	// Disable ctrl-c catching on Linux (and OS X?)
#ifdef __unix
	signal(SIGINT, SIG_DFL);
#endif

	// Load extensions.
	glewInit();

	printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
	printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
	printf("OpenGL Version: %s\n", glGetString(GL_VERSION)); 

	// == Initialize OpenGL ==

	*appshader = compile_shader (vertexshader, fragmentshader);
	glUseProgram(*appshader);

	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};

	GLuint VBOs[1];
	glGenBuffers(1, VBOs);

	glGenVertexArrays(1, &VAOs[0]);

	glBindVertexArray(VAOs[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) 0);
	glEnableVertexAttribArray(0);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*) (3 * sizeof(float)));
	glEnableVertexAttribArray(5);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	// two triangles draw a quad from -1,-1 to 1,1
	float quadvertices[] = {
		0, 1,
		0, 0,
		1, 0,

		0, 1,
		1, 1,
		1, 0
	};
	GLuint distortionVBO[1];
	glGenBuffers(1, distortionVBO);

	glGenVertexArrays(1, &VAOs[1]);

	glBindVertexArray(VAOs[1]);
	glBindBuffer(GL_ARRAY_BUFFER, distortionVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadvertices), quadvertices, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

static void compile_shader_src(GLuint shader, const char* src)
{
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);

	GLint status;
	GLint length;
	char log[4096] = {0};

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	glGetShaderInfoLog(shader, 4096, &length, log);
	if(status == GL_FALSE){
		printf("compile failed %s\n", log);
	}
}

GLuint compile_shader(const char* vertex, const char* fragment)
{
	// Create the handels
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint programShader = glCreateProgram();

	// Attach the shaders to a program handel.
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
	GLint length;
	char log[4096] = {0};

	glGetProgramiv(programShader, GL_LINK_STATUS, &status);
	glGetProgramInfoLog(programShader, 4096, &length, log);
	if(status == GL_FALSE){
		printf("link failed %s\n", log);
	}

	return programShader;
}

void create_fbo(int eye_width, int eye_height, GLuint* fbo, GLuint* color_tex, GLuint* depth_tex)
{
	glGenTextures(1, color_tex);
	glGenTextures(1, depth_tex);
	glGenFramebuffers(1, fbo);

	glBindTexture(GL_TEXTURE_2D, *color_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, eye_width, eye_height, 0, GL_RGBA, GL_UNSIGNED_INT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, *depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, eye_width, eye_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *color_tex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *depth_tex, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if(status != GL_FRAMEBUFFER_COMPLETE){
		printf("failed to create fbo %x\n", status);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
