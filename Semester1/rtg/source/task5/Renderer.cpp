


#include "Renderer.h"


Renderer::Renderer(GL::platform::Window& window)
	: BasicRenderer(window)
{
	glClearColor(0.1f, 0.3f, 1.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);

	window.attach(this);
}

void Renderer::resize(int width, int height)
{
	viewport_width = width;
	viewport_height = height;
}

void Renderer::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, viewport_width, viewport_height);

	swapBuffers();
}
