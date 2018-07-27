#include <stdio.h>
#include "simpleCompositor.h"

int main(int argc, char** argv) {
	initCompositor(0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (int i = 0; i < 10; i++) {

		glBindFramebuffer(GL_FRAMEBUFFER, left_fbo);
		glViewport(0, 0, eye_w, eye_h);
		glClearColor( i/10., 0, 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT );

		glBindFramebuffer(GL_FRAMEBUFFER, right_fbo);
		glViewport(0, 0, eye_w, eye_h);
		glClearColor( 0, i/10., 0, 1 );
		glClear( GL_COLOR_BUFFER_BIT );

		submitFrame();
		sleep(1);
	}
}
