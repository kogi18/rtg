


#ifndef INCLUDED_RENDERER
#define INCLUDED_RENDERER

#pragma once

#include <GL/gl.h>

#include <framework/BasicRenderer.h>


class Renderer : public BasicRenderer
{
private:
	int viewport_width;
	int viewport_height;

public:
	Renderer(const Renderer&) = delete;
	Renderer& operator =(const Renderer&) = delete;

	Renderer(GL::platform::Window& window);

	void resize(int width, int height);
	void render();
};

#endif  // INCLUDED_RENDERER
